#include <fstream>
#include <vector>
#include <cassert>
#include <algorithm>

#include <unordered_map>
#include <build_info.hpp>

#include <iostream>
using std::cout;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "fast_ig_tools.hpp"
#include "ig_final_alignment.hpp"

#include <seqan/seq_io.h>
using seqan::Dna5String;
using seqan::SeqFileIn;
using seqan::SeqFileOut;
using seqan::CharString;

std::pair<std::unordered_map<std::string, size_t>, std::vector<std::string>> read_rcm_file_string(const std::string &file_name) {
    std::ifstream rcm(file_name.c_str());

    std::unordered_map<std::string, size_t> result;
    std::unordered_map<std::string, size_t> barcode2num;

    std::string id, target;
    std::vector<std::string> targets;
    size_t num = 0;
    while (rcm >> id >> target) {
        if (barcode2num.count(target) == 0) { // TODO Use iterator here??
            barcode2num[target] = num++;
            targets.push_back(target);
            VERIFY(targets.size() == num);
        }

        result[id] = barcode2num[target];
    }

    return { result, targets };
}


template<typename T = seqan::Dna5>
void split_component(const std::vector<seqan::String<T>> &reads,
                     const std::vector<size_t> &indices,
                     std::vector<std::pair<seqan::String<T>, std::vector<size_t>>> &out,
                     size_t max_votes = 1,
                     bool discard = false) {
    if (indices.size() == 0) {
        return;
    }

    if (indices.size() == 1) {
        out.push_back({ reads[indices[0]], indices });
        return;
    }

    using namespace seqan;

    String<ProfileChar<T>> profile;

    size_t len = length(reads[indices[0]]);
    for (size_t i : indices) {
        VERIFY(length(reads[i]) == len);
    }

    resize(profile, len);

    for (size_t i : indices) {
        const auto &read = reads[i];
        for (size_t j = 0; j < length(read); ++j) {
            profile[j].count[ordValue(read[j])] += 1;
        }
    }

    // Find secondary votes
    struct PositionVote {
        size_t votes;
        size_t letter;
        size_t position;
        bool operator<(const PositionVote &b) const {
            return votes < b.votes;
        }
    };

    std::vector<PositionVote> secondary_votes;
    for (size_t j = 0; j < len; ++j) {
        std::vector<std::pair<size_t, size_t>> v;
        for (size_t k = 0; k < 4; ++k) {
            v.push_back({ profile[j].count[k], k });
        }

        // Use nth element here???
        std::sort(v.rbegin(), v.rend());
        secondary_votes.push_back({ v[1].first, v[1].second, j });
    }
    auto maximal_mismatch = *std::max_element(secondary_votes.cbegin(), secondary_votes.cend());

    size_t position = maximal_mismatch.position;
    size_t letter = maximal_mismatch.letter;
    size_t votes = maximal_mismatch.votes;

    INFO("VOTES: " << votes << " POSITION: " << position)
    if (votes <= max_votes) {
        // call consensus from this string
        seqan::String<T> consensus;
        for (size_t i = 0; i < length(profile); ++i) {
            size_t idx = getMaxIndex(profile[i]);
            if (idx < ValueSize<T>::VALUE) {  // is not gap  TODO Check it!!
                appendValue(consensus, T(getMaxIndex(profile[i])));
            }
        }

        out.push_back({ consensus, indices });
        return;
    }

    std::vector<size_t> indices1, indices2;
    for (size_t i : indices) {
        if (reads[i][position] == letter) {
            indices2.push_back(i);
        } else {
            indices1.push_back(i);
        }
    }

    INFO("Component splitted " << indices1.size() << " + " << indices2.size());
    split_component(reads, indices1, out);
    if (discard) {
        for (size_t index : indices2) {
            out.push_back({ reads[index], { index } });
        }
    } else {
        split_component(reads, indices2, out);
    }
}



template<typename T = seqan::Dna5>
std::vector<std::pair<seqan::String<T>, std::vector<size_t>>> split_component(const std::vector<seqan::String<T>> &reads,
                                                                              const std::vector<size_t> &indices,
                                                                              size_t max_votes = 1,
                                                                              bool discard = false) {
    std::vector<std::pair<seqan::String<T>, std::vector<size_t>>> result;
    split_component(reads, indices, result, max_votes, discard);

    return result;
}

int main(int argc, char **argv) {
    segfault_handler sh;
    perf_counter pc;
    create_console_logger("");

    INFO("Command line: " << join_cmd_line(argc, argv));

    int nthreads = 4;
    std::string reads_file = "input.fa";
    std::string output_file = "repertoire.fa";
    std::string rcm_file = "cropped.rcm";
    std::string output_rcm_file = "cropped.rcm";
    std::string config_file = "";
    size_t max_votes = 1;
    bool discard = false;

    // Parse cmd-line arguments
    try {
        // Declare a group of options that will be
        // allowed only on command line
        po::options_description generic("Generic options");
        generic.add_options()
            ("version,v", "print version string")
            ("help,h", "produce help message")
            ("config,c", po::value<std::string>(&config_file)->default_value(config_file),
             "name of a file of a configuration")
            ("input-file,i", po::value<std::string>(&reads_file),
             "name of the input file (FASTA|FASTQ)")
            ("output-file,o", po::value<std::string>(&output_file)->default_value(output_file),
             "output file for final repertoire")
            ("rcm-file,R", po::value<std::string>(&rcm_file)->default_value(rcm_file),
             "input RCM-file")
            ("output-rcm-file,M", po::value<std::string>(&output_rcm_file)->default_value(output_rcm_file),
             "input RCM-file");

        // Declare a group of options that will be
        // allowed both on command line and in
        // config file
        po::options_description config("Configuration");
        config.add_options()
            ("threads,t", po::value<int>(&nthreads)->default_value(nthreads),
             "the number of parallel threads")
            ("max-votes,V", po::value<size_t>(&max_votes)->default_value(max_votes),
             "max secondary votes threshold")
            ("disacrd,D", po::value<bool>(&discard)->default_value(discard),
             "whether to discard secondary votes")
            ;

        // Hidden options, will be allowed both on command line and
        // in config file, but will not be shown to the user.
        po::options_description hidden("Hidden options");
        hidden.add_options()
            ("help-hidden", "show all options, including developers' ones")
            ;

        po::options_description cmdline_options("All command line options");
        cmdline_options.add(generic).add(config).add(hidden);

        po::options_description config_file_options;
        config_file_options.add(config).add(hidden);

        po::options_description visible("Allowed options");
        visible.add(generic).add(config);

        po::positional_options_description p;
        p.add("input-file", 1);

        po::variables_map vm;
        store(po::command_line_parser(argc, argv).
              options(cmdline_options).run(), vm);
        notify(vm);

        if (vm.count("help-hidden")) {
            cout << cmdline_options << std::endl;
            return 0;
        }

        if (vm.count("help")) {
            cout << visible << std::endl;
            return 0;
        }

        if (vm.count("version")) {
            cout << bformat("IG Component Splitter, part of IgReC version %s; git version: %s") % build_info::version % build_info::git_hash7 << std::endl;
            return 0;
        }

        if (vm.count("config-file")) {
            std::string config_file = vm["config-file"].as<std::string>();

            std::ifstream ifs(config_file.c_str());
            if (!ifs) {
                cout << "can not open config file: " << config_file << "\n";
                return 0;
            } else {
                store(parse_config_file(ifs, config_file_options), vm);
                // reparse cmd line again for update config defaults
                store(po::command_line_parser(argc, argv).
                      options(cmdline_options).positional(p).run(), vm);
            }
        }

        try {
            notify(vm);
        } catch (po::error &e) {
            cout << "Parser error: " << e.what() << std::endl;
        }
    } catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    INFO("Command line: " << join_cmd_line(argc, argv));
    INFO("Input files: " << reads_file << ", " << rcm_file);

    std::vector<Dna5String> input_reads;
    std::vector<CharString> input_ids;
    SeqFileIn seqFileIn_input(reads_file.c_str());

    INFO("Reading input reads starts");
    readRecords(input_ids, input_reads, seqFileIn_input);
    INFO(input_reads.size() << " reads were extracted from " << reads_file);

    std::vector<size_t> component_indices;
    component_indices.resize(input_reads.size());

    INFO("Reading read-cluster map starts");
    auto rcm = read_rcm_file_string(rcm_file);
    for (size_t i = 0; i < input_reads.size(); ++i) {
        const char *id = toCString(input_ids[i]);
        VERIFY(rcm.first.count(id));
        component_indices[i] = rcm.first[id];
    }

    INFO(rcm.second.size() << " clusters were extracted from " << rcm_file);

    std::vector<std::vector<size_t>> component2id(rcm.second.size());
    for (size_t i = 0; i < component_indices.size(); ++i) {
        component2id[component_indices[i]].push_back(i);
    }

    size_t max_component_size = 0;
    for (const auto &_ : component2id) {
        max_component_size = std::max(max_component_size, _.size());
    }
    INFO(bformat("Size of maximal cluster: %d") % max_component_size);

    omp_set_num_threads(nthreads);
    INFO(bformat("Computation of consensus using %d threads starts") % nthreads);

    using T = seqan::Dna5;

    std::vector<std::vector<std::pair<seqan::String<T>, std::vector<size_t>>>> results(component2id.size());
    SEQAN_OMP_PRAGMA(parallel for schedule(dynamic, 8))
    for (size_t comp = 0; comp < component2id.size(); ++comp) {
        results[comp] = split_component(input_reads, component2id[comp], max_votes, discard);
    }

    INFO("Saving results");

    SeqFileOut seqFileOut_output(output_file.c_str());

    std::ofstream out_rcm(output_rcm_file.c_str());

    for (size_t comp = 0; comp < results.size(); ++comp) {
        const auto &result = results[comp];
        for (size_t i = 0; i < result.size(); ++i) {
            // TODO Add optionally continuous numbering
            bformat fmt("cluster___%dX%d___size___%d");
            fmt % comp % i % result[i].second.size();
            std::string id = fmt.str();

            seqan::writeRecord(seqFileOut_output, id, result[i].first);
            for (size_t read_index : result[i].second) {
                std::string read_id = seqan::toCString(input_ids[read_index]);
                out_rcm << read_id << "\t" << comp << "X" << i << "\n";
            }
        }
    }

    INFO("Final repertoire was written to " << output_file);
    INFO("Final RCM was written to " << output_rcm_file);

    INFO("Running time: " << running_time_format(pc));

    return 0;
}

// vim: ts=4:sw=4
