#pragma once

// Main IR header - aggregates all parsing components
#include "diagnostics.hpp"
#include "tokenizer.hpp"
#include "ir_types.hpp"
#include "parser.hpp"
#include "semantic.hpp"

// Backward compatibility: import into global namespace
using namespace forma;

// This file provides backward compatibility by including all components
// Users can include just ir.hpp and get everything, or include specific headers for faster compilation
