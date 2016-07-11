#include "logger/logger.hpp"
#include "left_event_shms_calculator.hpp"

using namespace vdj_labeler;

int LeftEventSHMsCalculator::ComputeNumberCleavedSHMs(alignment_utils::ImmuneGeneReadAlignmentPtr gene_alignment,
                                                      size_t cleavage_length) {
    TRACE("Computation of # SHMs in left cleavage of length " << cleavage_length);
    // Gene is Subject.
    // If there is (already) the cleavage, then the gene_alignment->StartSubjectPosition() != 0.
    // Already -- means the cleavage detecting while aligning.
    assert(cleavage_length >= gene_alignment->StartSubjectPosition());
    size_t rel_cleavage_len = cleavage_length - gene_alignment->StartSubjectPosition();
    auto& alignment = gene_alignment->Alignment();

    auto &gene = seqan::row(alignment, 0);
    auto &read = seqan::row(alignment, 1);
    int num_shms = 0;
    int cur_cleavage = 0;
    for(size_t i = 0;
        i < seqan::length(gene) and cur_cleavage != static_cast<int>(rel_cleavage_len);
        i++)
    {
        if(read[i] != '-')
            cur_cleavage++;
        if(gene[i] != read[i])
            num_shms++;
    }
    TRACE("Cleavage length: " << cleavage_length << ", rel cleavage len: " << rel_cleavage_len);
    TRACE("#SHMs: -" << num_shms);
    return -1 * num_shms;
}

int LeftEventSHMsCalculator::ComputeNumberPalindromeSHMs(alignment_utils::ImmuneGeneReadAlignmentPtr gene_alignment,
                                                         size_t palindrome_length) {
    TRACE("Computation of #SHMs in left palindrome of length " << palindrome_length);
    // if gene has alignment to read with gaps at the end, we can not compute
    assert(gene_alignment->StartSubjectPosition() == 0);
    int num_shms = 0;
    for(size_t i = 0; i < palindrome_length; i++) {
        size_t gene_pos = i;
        size_t read_pos = gene_alignment->StartQueryPosition() - 1 - i;
        seqan::DnaString nucl(gene_alignment->subject().seq()[gene_pos]);
        seqan::complement(nucl);
        // nucl has len 1.
        if(nucl[0] != gene_alignment->query().seq[read_pos])
            num_shms++;
    }
    TRACE("#SHMs: +" << num_shms);
    return num_shms;
}

int LeftEventSHMsCalculator::ComputeNumberSHMs(alignment_utils::ImmuneGeneReadAlignmentPtr gene_alignment,
                                               int left_cleavage_length,
                                               int) {
    if(left_cleavage_length == 0)
        return 0;
    // if gene was cleaved, number of SHMs would be 0 or negative
    // since some unaligned nucleotides were cleaved
    if(left_cleavage_length > 0)
        return ComputeNumberCleavedSHMs(gene_alignment, size_t(left_cleavage_length));
    // if gene contains palindrome, number of SHMs would be 0 or positive
    // since some nucleotides in palindrome are mutated
    return ComputeNumberPalindromeSHMs(gene_alignment, size_t(left_cleavage_length * -1));
}

int LeftEventSHMsCalculator::ComputeNumberSHMsForLeftEvent(alignment_utils::ImmuneGeneReadAlignmentPtr gene_alignment,
                                                           int left_cleavage_length) {
    if(left_cleavage_length == 0)
        return 0;
    // if gene was cleaved, number of SHMs would be 0 or negative
    // since some unaligned nucleotides were cleaved
    if(left_cleavage_length > 0)
        return ComputeNumberCleavedSHMs(gene_alignment, size_t(left_cleavage_length));
    // if gene contains palindrome, number of SHMs would be 0 or positive
    // since some nucleotides in palindrome are mutated
    return ComputeNumberPalindromeSHMs(gene_alignment, size_t(left_cleavage_length * -1));
}