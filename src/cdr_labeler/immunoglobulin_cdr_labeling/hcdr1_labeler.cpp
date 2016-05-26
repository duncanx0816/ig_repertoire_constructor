#include "hcdr1_labeler.hpp"

namespace cdr_labeler {
    bool PositionIsCys(seqan::Dna5String seq, size_t pos) {
        return seq[pos] == 'T' and seq[pos + 1] == 'G' and (seq[pos + 2] == 'T' or seq[pos + 2] == 'C');
    }

    bool PositionIsTrp(seqan::Dna5String seq, size_t pos) {
        return seq[pos] == 'T' and seq[pos + 1] == 'G' and seq[pos + 2] == 'G';
    }

    size_t abs_diff(size_t n1, size_t n2) {
        if(n1 > n2)
            return n1 - n2;
        return n2 - n1;
    }

    size_t HCDR1Labeler::ComputeStartPosition(const germline_utils::ImmuneGene &immune_gene) {
        size_t aa_length = immune_gene.length() / 3;
        size_t cdr1_start = size_t(-1);
        for(size_t i = 0; i < aa_length; i++) {
            if(PositionIsCys(immune_gene.seq(), i * 3)) {
                cdr1_start = i * 3 + 12;
                break;
            }
        }
        if(abs_diff(params_.start_pos, cdr1_start) <= params_.start_shift)
            return cdr1_start;
        return size_t(-1);
    }

    size_t HCDR1Labeler::ComputeEndPosition(const germline_utils::ImmuneGene &immune_gene,
                                            size_t start_pos) {
        VERIFY_MSG(start_pos % 3 == 0, "Start position of HCDR1 " << start_pos << " is not in frame");
        size_t aa_length = immune_gene.length() / 3;
        size_t end_pos = size_t(-1);
        for(size_t i = start_pos / 3 + 1; i < aa_length; i++)
            if(PositionIsTrp(immune_gene.seq(), i * 3)) {
                end_pos = i * 3 - 3;
                break;
            }
        if(end_pos < start_pos)
            return size_t(-1);
        size_t cdr1_length = end_pos - start_pos + 1;
        if(cdr1_length >= params_.min_length and cdr1_length <= params_.max_length)
            return end_pos;
        return size_t(-1);
    }

    CDRRange HCDR1Labeler::ComputeRange(const germline_utils::ImmuneGene &immune_gene, CDRRange) {
        size_t start_pos = ComputeStartPosition(immune_gene);
        size_t end_pos = ComputeEndPosition(immune_gene, start_pos);
        return CDRRange(start_pos, end_pos);
    }
}