//
// Created by Andrew Bzikadze on 4/11/17.
//

#pragma once

#include <cstddef>
#include <random>

#include "ig_simulator_utils.hpp"
#include "random_index.hpp"
#include "ig_simulator_config.hpp"

namespace ig_simulator {

class AbstractTreeSizeGenerator {
public:
    AbstractTreeSizeGenerator() = default;
    AbstractTreeSizeGenerator(const AbstractTreeSizeGenerator&) = delete;
    AbstractTreeSizeGenerator(AbstractTreeSizeGenerator&&) = delete;
    AbstractTreeSizeGenerator& operator=(const AbstractTreeSizeGenerator&) = delete;
    AbstractTreeSizeGenerator& operator=(AbstractTreeSizeGenerator&&) = delete;

    virtual size_t Generate() = 0;

    virtual ~AbstractTreeSizeGenerator() { }
};

using AbstractTreeSizeGeneratorCPtr = std::unique_ptr<const AbstractTreeSizeGenerator>;

class GeometricTreeSizeGenerator final : public AbstractTreeSizeGenerator {
private:
    std::geometric_distribution<size_t> distribution;

public:
    GeometricTreeSizeGenerator(double lambda):
        AbstractTreeSizeGenerator(),
        distribution(check_numeric_positive(lambda))
    { }

    GeometricTreeSizeGenerator(const TreeSizeGeneratorParams::GeometricParams& params):
        GeometricTreeSizeGenerator(params.lambda)
    { }

    size_t Generate() override {
        return distribution(MTSingleton::GetInstance());
    }
};

AbstractTreeSizeGeneratorCPtr get_tree_size_generator(const TreeSizeGeneratorParams& config);

} // End namespace ig_simulator
