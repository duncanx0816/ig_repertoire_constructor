#include <memory>

#include "logger/logger.hpp"
#include "vdj_hits.hpp"
#include "immune_gene_alignment_converter.hpp"
#undef NDEBUG
#include <cassert>

namespace vdj_labeler {

VDJHits::VDJHits(const core::Read* read_ptr):
    read_ptr_(read_ptr),
    v_hits_(germline_utils::SegmentType::VariableSegment, read_ptr),
    d_hits_(read_ptr),
    j_hits_(germline_utils::SegmentType::JoinSegment, read_ptr)
{ }

VDJHits::VDJHits(const core::Read* read_ptr,
                 const std::vector<vj_finder::VGeneHit> &v_hits,
                 const std::vector<vj_finder::JGeneHit> &j_hits):
    read_ptr_(read_ptr),
    v_hits_(germline_utils::SegmentType::VariableSegment, read_ptr, v_hits),
    d_hits_(read_ptr),
    j_hits_(germline_utils::SegmentType::JoinSegment, read_ptr, j_hits)
{ }

VDJHits::VDJHits(const core::Read* read_ptr,
                 const std::vector<vj_finder::VGeneHit> &v_hits,
                 const std::vector<vj_finder::JGeneHit> &j_hits,
                 AbstractDGeneHitsCalculator &d_gene_calculator) :
        read_ptr_(read_ptr),
        v_hits_(germline_utils::SegmentType::VariableSegment, read_ptr, v_hits),
        d_hits_(d_gene_calculator.ComputeDHits(read_ptr, v_hits, j_hits)),
        j_hits_(germline_utils::SegmentType::JoinSegment, read_ptr, j_hits)
{ }

VDJHits::VDJHits(const vj_finder::VJHits &vj_hits):
    VDJHits(&(vj_hits.Read()),
            vj_hits.VHits(),
            vj_hits.JHits())
{ }


VDJHits::VDJHits(const vj_finder::VJHits &vj_hits, AbstractDGeneHitsCalculator &d_gene_calculator):
    // TODO After refactoring, vj_hits will contain a pointer
    VDJHits(&vj_hits.Read(),
            vj_hits.VHits(),
            vj_hits.JHits(),
            d_gene_calculator)
{ }


// void VDJHits::AddIgGeneAlignment(alignment_utils::ImmuneGeneReadAlignment alignment) {
//     germline_utils::SegmentType segment_type = alignment.Subject().GeneType().Segment();
//     AddIgGeneAlignment(segment_type, std::move(alignment));
// }
//
// void VDJHits::AddIgGeneAlignment(const germline_utils::SegmentType &segment_type,
//                                  alignment_utils::ImmuneGeneReadAlignment alignment) {
//     if (segment_type == germline_utils::SegmentType::VariableSegment)
//         v_hits_.AddHit(std::move(alignment));
//     else if (segment_type == germline_utils::SegmentType::DiversitySegment)
//         d_hits_.AddHit(std::move(alignment));
//     else if (segment_type == germline_utils::SegmentType::JoinSegment)
//         j_hits_.AddHit(std::move(alignment));
// }

// const alignment_utils::ImmuneGeneReadAlignment &VDJHits::GetAlignmentByIndex(
//         const germline_utils::SegmentType &segment_type,
//         const size_t &index) const
// {
//     if (segment_type == germline_utils::SegmentType::VariableSegment) {
//         VERIFY(index < v_hits_.size());
//         return v_hits_[index];
//     }
//     else if (segment_type == germline_utils::SegmentType::DiversitySegment) {
//         VERIFY(index < d_hits_.size());
//         return d_hits_[index];
//     }
//     VERIFY(index < j_hits_.size());
//     return j_hits_[index];
// }

} // End namespace vdj_labeler