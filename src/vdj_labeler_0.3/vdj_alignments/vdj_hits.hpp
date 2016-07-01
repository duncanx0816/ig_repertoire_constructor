#pragma once

#include "alignment_utils/alignment_positions.hpp"
#include "alignment_utils/pairwise_alignment.hpp"
#include "germline_utils/germline_gene_type.hpp"
#include "read_archive.hpp"

namespace vdj_labeler {

typedef std::vector<alignment_utils::ImmuneGeneReadAlignmentPtr>::const_iterator hits_citerator;

class ImmuneGeneSegmentHits {
    germline_utils::SegmentType segment_type_;
    core::ReadPtr read_ptr_;
    std::vector<alignment_utils::ImmuneGeneReadAlignmentPtr> hits_;

public:
    ImmuneGeneSegmentHits(germline_utils::SegmentType gene_type, core::ReadPtr read_ptr) :
            segment_type_(gene_type),
            read_ptr_(read_ptr) { }

    void AddHit(alignment_utils::ImmuneGeneReadAlignmentPtr hit);

    size_t size() const { return hits_.size(); }

    hits_citerator cbegin() const  { return hits_.cbegin(); }

    hits_citerator cend() const { return hits_.cend(); }

    alignment_utils::ImmuneGeneReadAlignmentPtr operator[](size_t index);

    germline_utils::SegmentType GeneType() const { return segment_type_; }
};

typedef std::shared_ptr<ImmuneGeneSegmentHits> ImmuneGeneSegmentHitsPtr;

//------------------------------------------------------------

class VDJHits {
    core::ReadPtr read_ptr_;
    ImmuneGeneSegmentHits v_hits_;
    ImmuneGeneSegmentHits d_hits_;
    ImmuneGeneSegmentHits j_hits_;

public:
    VDJHits(core::ReadPtr read_ptr) :
            read_ptr_(read_ptr),
            v_hits_(germline_utils::SegmentType::VariableSegment, read_ptr),
            d_hits_(germline_utils::SegmentType::DiversitySegment, read_ptr),
            j_hits_(germline_utils::SegmentType::JoinSegment, read_ptr) { }

    void AddIgGeneAlignment(germline_utils::SegmentType gene_type,
                            alignment_utils::ImmuneGeneReadAlignmentPtr alignment_ptr);

    void AddIgGeneAlignment(alignment_utils::ImmuneGeneReadAlignmentPtr alignment_ptr);

    size_t VHitsNumber() const { return v_hits_.size(); }

    size_t DHitsNumber() const { return d_hits_.size(); }

    size_t JHitsNumber() const { return j_hits_.size(); }

    core::ReadPtr Read() const { return read_ptr_; }

    alignment_utils::ImmuneGeneReadAlignmentPtr GetAlignmentByIndex(germline_utils::SegmentType gene_type,
                                                                    size_t index);
};

typedef std::shared_ptr<VDJHits> VDJHitsPtr;

} // End namespace vdj_labeler