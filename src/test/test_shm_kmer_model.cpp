//
// Created by Andrew Bzikadze on 5/25/16.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <logger/log_writers.hpp>

#include "evolutionary_edge_alignment/evolutionary_edge_alignment.hpp"
#include "alignment_checker/no_gaps_alignment_checker.hpp"
#include "shm_config.hpp"
#include "alignment_cropper/upto_last_reliable_kmer_alignment_cropper.hpp"
#include "germline_alignment_reader/alignment_reader.hpp"
#include "mutation_strategies/trivial_strategy.hpp"
#include "mutation_strategies/no_k_neighbours.hpp"

void create_console_logger() {
    using namespace logging;
    logger *lg = create_logger("");
    lg->add_writer(std::make_shared<console_writer>());
    attach_logger(lg);
}

class AlignmentCheckerTest : public ::testing::Test {
public:
    void SetUp() { create_console_logger(); }
};

TEST_F(AlignmentCheckerTest, CheckingIsCorrect) {
    shm_config shm_config_ach_nogaps;
    std::string config_ach_nogaps = "configs/shm_kmer_model/configs.info";
    load(shm_config_ach_nogaps, config_ach_nogaps);
    NoGapsAlignmentChecker alch(shm_config_ach_nogaps.achp);

    ns_gene_alignment::EvolutionaryEdgeAlignment alignment1("AAA", "CCC", "id");
    ASSERT_TRUE(alch.check(alignment1));
    ns_gene_alignment::EvolutionaryEdgeAlignment alignment2("AAA--", "CCCAA", "id");
    ASSERT_FALSE(alch.check(alignment2));
    ns_gene_alignment::EvolutionaryEdgeAlignment alignment3("AAA--", "-CCAA", "id");
    ASSERT_FALSE(alch.check(alignment3));
}

class AlignmentCropperTest: public ::testing::Test {
public:
    void SetUp() { create_console_logger(); }
};

TEST_F(AlignmentCropperTest, CheckingIsCorrect) {
    shm_config shm_config_acr_last_rel_kmer;
    std::string config_acr_last_rel_kmer = "configs/shm_kmer_model/configs.info";
    load(shm_config_acr_last_rel_kmer, config_acr_last_rel_kmer);
    UptoLastReliableKmerAlignmentCropper alcr(shm_config_acr_last_rel_kmer.acrp.rkmp);

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("AAAAAAAAAAAAAAA",
                                                               "AAAAAAAAAAAAAAA", "id");
        alcr.crop(alignment);
        ASSERT_EQ(alignment.son(),    "AAAAAAAAAAAAAAA");
        ASSERT_EQ(alignment.parent(), "AAAAAAAAAAAAAAA");
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("ABAAAAAAAAACAAA",
                                                               "ABAAAAAAAAACAAA", "id");
        alcr.crop(alignment);
        ASSERT_EQ(alignment.son(),    "ABAAAAAAAAACAAA");
        ASSERT_EQ(alignment.parent(), "ABAAAAAAAAACAAA");
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("BBAAAAAAAAACAAA",
                                                               "ABAAAAAAAAACAAA", "id", 1, 1, 1, 1);
        ASSERT_EQ(alignment.cdr1_start(), 1);
        ASSERT_EQ(alignment.cdr1_end(), 1);
        ASSERT_EQ(alignment.cdr2_start(), 1);
        ASSERT_EQ(alignment.cdr2_end(), 1);
        alcr.crop(alignment);
        ASSERT_EQ(alignment.son(),    "BAAAAAAAAACAAA");
        ASSERT_EQ(alignment.parent(), "BAAAAAAAAACAAA");
        ASSERT_EQ(alignment.cdr1_start(), 0);
        ASSERT_EQ(alignment.cdr1_end(), 0);
        ASSERT_EQ(alignment.cdr2_start(), 0);
        ASSERT_EQ(alignment.cdr2_end(), 0);
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("ABAAAAAAAAACAAA",
                                                               "ABAAAAAAAAACAAB", "id");
        alcr.crop(alignment);
        ASSERT_EQ(alignment.son(),    "ABAAAAAAAAACAA");
        ASSERT_EQ(alignment.parent(), "ABAAAAAAAAACAA");
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("ABAAAAAAAAACAAA",
                                                               "CBAAAAAAAAACAAB", "id", 1, 1, 1, 1);
        ASSERT_EQ(alignment.cdr1_start(), 1);
        ASSERT_EQ(alignment.cdr1_end(), 1);
        ASSERT_EQ(alignment.cdr2_start(), 1);
        ASSERT_EQ(alignment.cdr2_end(), 1);
        alcr.crop(alignment);
        ASSERT_EQ(alignment.son(),    "BAAAAAAAAACAA");
        ASSERT_EQ(alignment.parent(), "BAAAAAAAAACAA");
        ASSERT_EQ(alignment.cdr1_start(), 0);
        ASSERT_EQ(alignment.cdr1_end(), 0);
        ASSERT_EQ(alignment.cdr2_start(), 0);
        ASSERT_EQ(alignment.cdr2_end(), 0);
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("ABAAACAAAAACAAA",
                                                               "CBAAADAAAAACAAB", "id", 6, 6, 6, 6);
        ASSERT_EQ(alignment.cdr1_start(), 6);
        ASSERT_EQ(alignment.cdr1_end(), 6);
        ASSERT_EQ(alignment.cdr2_start(), 6);
        ASSERT_EQ(alignment.cdr2_end(), 6);
        alcr.crop(alignment);
        ASSERT_EQ(alignment.son(),     "AAAAACAA");
        ASSERT_EQ(alignment.parent(),  "AAAAACAA");
        ASSERT_EQ(alignment.cdr1_start(), 0);
        ASSERT_EQ(alignment.cdr1_end(), 0);
        ASSERT_EQ(alignment.cdr2_start(), 0);
        ASSERT_EQ(alignment.cdr2_end(), 0);
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("CAGGTGCAGCTGFTGC",
                                                               "CAGCCGCAGCTGGTGC", "id", 5, 5, 5, 5);
        ASSERT_EQ(alignment.cdr1_start(), 5);
        ASSERT_EQ(alignment.cdr1_end(), 5);
        ASSERT_EQ(alignment.cdr2_start(), 5);
        ASSERT_EQ(alignment.cdr2_end(), 5);
        alcr.crop(alignment);
        ASSERT_EQ(alignment.son(),    "GCAGCTG");
        ASSERT_EQ(alignment.parent(), "GCAGCTG");
        ASSERT_EQ(alignment.cdr1_start(), 0);
        ASSERT_EQ(alignment.cdr1_end(), 0);
        ASSERT_EQ(alignment.cdr2_start(), 0);
        ASSERT_EQ(alignment.cdr2_end(), 0);
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("CCGFTGCAGCTGFTTC",
                                                               "CAGhCGCAGCTGGTGC", "id", 5, 5, 5, 5);
        ASSERT_EQ(alignment.cdr1_start(), 5);
        ASSERT_EQ(alignment.cdr1_end(), 5);
        ASSERT_EQ(alignment.cdr2_start(), 5);
        ASSERT_EQ(alignment.cdr2_end(), 5);
        alcr.crop(alignment);
        ASSERT_EQ(alignment.son(),    "GCAGCTG");
        ASSERT_EQ(alignment.parent(), "GCAGCTG");
        ASSERT_EQ(alignment.cdr1_start(), 0);
        ASSERT_EQ(alignment.cdr1_end(), 0);
        ASSERT_EQ(alignment.cdr2_start(), 0);
        ASSERT_EQ(alignment.cdr2_end(), 0);
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("AAAAAA",
                                                               "AAAAAC", "id");
        ASSERT_EQ(alignment.cdr1_start(), 0);
        ASSERT_EQ(alignment.cdr1_end(), 0);
        ASSERT_EQ(alignment.cdr2_start(), 0);
        ASSERT_EQ(alignment.cdr2_end(), 0);
        alcr.crop(alignment);
        ASSERT_EQ(alignment.son(),    "AAAAA");
        ASSERT_EQ(alignment.parent(), "AAAAA");
        ASSERT_EQ(alignment.cdr1_start(), 0);
        ASSERT_EQ(alignment.cdr1_end(), 0);
        ASSERT_EQ(alignment.cdr2_start(), 0);
        ASSERT_EQ(alignment.cdr2_end(), 0);
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("CAGACGCAGCTGGTGF",
                                                               "AAGACGDAGCTGGTGC", "id", 1, 1, 1, 1);
        ASSERT_EQ(alignment.cdr1_start(), 1);
        ASSERT_EQ(alignment.cdr1_end(), 1);
        ASSERT_EQ(alignment.cdr2_start(), 1);
        ASSERT_EQ(alignment.cdr2_end(), 1);
        alcr.crop(alignment);
        ASSERT_EQ(alignment.son(),    "AGACGDAGCTGGTG");
        ASSERT_EQ(alignment.parent(), "AGACGCAGCTGGTG");
        ASSERT_EQ(alignment.cdr1_start(), 0);
        ASSERT_EQ(alignment.cdr1_end(), 0);
        ASSERT_EQ(alignment.cdr2_start(), 0);
        ASSERT_EQ(alignment.cdr2_end(), 0);
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("CAGGTGCAGZZGGTGCAGTCTGGGAGTGAACTGAAGAAGCCTGGGGCCTCAGTGAAGGTCTCCTGCAAGGCTTCTGCCGTCTATTACTGCACGAGACA",
                                                               "CAGGZGCAGCTGGTZCAGZCTGGGGCTGAGGTGAAGAAGCCTGGGGCCTCAGTGAAGGTCTCCTGCAAGGCTTCGGTCGTGTATTACTGTGZGAGAGA", "id", 19, 19, 19, 19);
        ASSERT_EQ(alignment.cdr1_start(), 19);
        ASSERT_EQ(alignment.cdr1_end(), 19);
        ASSERT_EQ(alignment.cdr2_start(), 19);
        ASSERT_EQ(alignment.cdr2_end(), 19);
        alcr.crop(alignment);
        ASSERT_EQ(alignment.son(),    "CTGGGGCTGAGGTGAAGAAGCCTGGGGCCTCAGTGAAGGTCTCCTGCAAGGCTTCGGTCGTGTATTACTG");
        ASSERT_EQ(alignment.parent(), "CTGGGAGTGAACTGAAGAAGCCTGGGGCCTCAGTGAAGGTCTCCTGCAAGGCTTCTGCCGTCTATTACTG");
        ASSERT_EQ(alignment.cdr1_start(), 0);
        ASSERT_EQ(alignment.cdr1_end(), 0);
        ASSERT_EQ(alignment.cdr2_start(), 0);
        ASSERT_EQ(alignment.cdr2_end(), 0);
    }
}

class AlignmentReaderTest: public ::testing::Test {
public:
    void SetUp() { create_console_logger(); }
};


TEST_F(AlignmentReaderTest, CheckingIsCorrect) {
    shm_config shm_config_ar;
    std::string config_ar = "configs/shm_kmer_model/configs.info";

    load(shm_config_ar, config_ar);

    ns_alignment_reader::AlignmentReader alignment_reader(shm_config_ar.io.input.v_alignments,
                                                          shm_config_ar.io.input.cdr_details,
                                                          shm_config_ar.achp,
                                                          shm_config_ar.acrp);

    auto alignments = alignment_reader.read_alignments();
    ASSERT_EQ(alignments.size(), 27);
    ASSERT_EQ(alignments[0].son(),    "CAGGTGCAGCTGGTGGAGTCTGGGGGAGGCGTGGTCCAGCCTGGGAGGTCCCTGAGACTCTCCTGTGCAGCCTCTGGATTCACCTTCAGTAGCTATGCTATGCACTGGGTCCGCCAGGCTCCAGGCAAGGGGCTGGAGTGGGTGGCAGTTATATCATATGATGGAAGCAATAAATACTACGCAGACTCCGTGAAGGGCCGATTCACCATCTCCAGAGACAATTCCAAGAACACGCTGTATCTGCAAATGAACAGCCTGAGAGCTGAGGACACGGCTGTGTATTACTGTGCGAGAGA");
    ASSERT_EQ(alignments[0].parent(), "CAGGTGCAGCTGGTGGAGTCTGGGGGAGGCGTGGTCCAGCCTGGGAGGTCCCTGAGACTCTCCTGTGCAGCCTCTGGATTCACCTTCAGTAGCTATGCTATGCACTGGGTCCGCCAGGCTCCAGGCAAGGGGCTGGAGTGGGTGGCAGTTATATCATATGATGGAAGCAATAAATACTACGCAGACTCCGTGAAGGGCCGATTCACCATCTCCAGAGACAATTCCAAGAACACGCTGTATCTGCAAATGAACAGCCTGAGAGCTGAGGACACGGCTGTGTATTACTGTGCGAGAGA");
    ASSERT_EQ(alignments[0].gene_id(), "INDEX:1|GENE:IGHV3-30-3*01|START_POS:0|END_POS:295|CHAIN_TYPE:IGH");
    ASSERT_EQ(alignments[0].cdr1_start(), 76);
    ASSERT_EQ(alignments[0].cdr1_end(), 99);
    ASSERT_EQ(alignments[0].cdr2_start(), 151);
    ASSERT_EQ(alignments[0].cdr2_end(), 174);
}

class MutationStrategiesTest: public ::testing::Test {
public:
    void SetUp() { create_console_logger(); }
};

TEST_F(MutationStrategiesTest, CheckNoKNeighbour) {
    shm_config shm_config_ms;
    std::string config_ms_nkn = "configs/shm_kmer_model/configs.info";
    load(shm_config_ms, config_ms_nkn);

    NoKNeighboursMutationStrategy ms_nkn(shm_config_ms.mfp);

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("AATCGGAAAA", "AACCGGTTAA", "id");
        auto rel_pos = ms_nkn.calculate_relevant_positions(alignment);
        // ASSERT_THAT(v, ElementsAre(2, 5, 6, 7, 8, 9));
        ASSERT_EQ(rel_pos.size(), 1);
        ASSERT_EQ(rel_pos[0], 2);
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("AATCGGAAAA", "AACCGGAAAA", "id");
        auto rel_pos = ms_nkn.calculate_relevant_positions(alignment);
        // ASSERT_THAT(v, ElementsAre(2, 5, 6, 7, 8, 9));
        ASSERT_EQ(rel_pos.size(), 4);
        ASSERT_EQ(rel_pos[0], 2);
        for (size_t i = 1; i < 4; ++i)
            ASSERT_EQ(rel_pos[i], i + 4);
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("CCTCCATCGGTTTCTG", "TCTCCAACGTTTTCTG", "id");
        auto rel_pos = ms_nkn.calculate_relevant_positions(alignment);
        EXPECT_THAT(rel_pos, testing::ElementsAre(3, 6, 9, 12, 13));
    }

    {
        ns_gene_alignment::EvolutionaryEdgeAlignment alignment("CCTCCATCGGTTTCTGTGCATGACGA", "TTTCCAACGTTTTCTGTGCACGAGGA", "id");
        auto rel_pos = ms_nkn.calculate_relevant_positions(alignment);
        EXPECT_THAT(rel_pos, testing::ElementsAre(6, 9, 12, 13, 14, 15, 16, 17, 20, 23));
    }
}
