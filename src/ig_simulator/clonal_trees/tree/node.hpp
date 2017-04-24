//
// Created by Andrew Bzikadze on 4/9/17.
//

#pragma once

#include <cstddef>
#include <vector>
#include <utility>
#include <string>

#include "seqan/basic.h"


namespace ig_simulator {

class Node {
public:
    using SHM_Vector = std::vector<std::tuple<size_t, seqan::Dna5, seqan::Dna>>;

private:
    size_t parent_ind;

    // We store only SHMs "on the edge" from the parent
    SHM_Vector shms;
    bool included;

public:
    Node(size_t parent_ind = size_t(-1),
         SHM_Vector&& shms = {},
         bool included = true):
        parent_ind(parent_ind),
        shms(std::move(shms)),
        included(included)
    { }

    Node(const Node&) = default;
    Node(Node&&) = default;
    Node& operator=(const Node&) = default;
    Node& operator=(Node&&) = default;

    size_t ParentInd() const { return parent_ind; }
    const SHM_Vector& SHMs() const { return shms; }

    void Exclude() { included = false; }

    bool IsIncluded() const { return included; }
};

} // End namespace ig_simulator
