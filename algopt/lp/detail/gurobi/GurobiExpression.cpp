// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "algopt/lp/detail/gurobi/GurobiExpression.h"

#ifdef REBALANCER_USE_GUROBI

#include "algopt/lp/detail/gurobi/GurobiRelation.h"

#include <fmt/format.h>
#include <folly/container/irange.h>

#include <cmath>
#include <ostream>

namespace facebook::algopt::lp::detail {

GurobiExpression::GurobiExpression(const GRBLinExpr& expression)
    : expression_(expression) {}

GurobiExpression::GurobiExpression(
    const std::variant<GRBLinExpr, GRBQuadExpr>& expression)
    : expression_(expression) {}

std::shared_ptr<ExpressionImpl> GurobiExpression::clone() const try {
  return std::make_shared<GurobiExpression>(expression_);
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

void GurobiExpression::add(double constant) try {
  if (std::holds_alternative<GRBLinExpr>(expression_)) {
    std::get<GRBLinExpr>(expression_) += constant;
  } else {
    std::get<GRBQuadExpr>(expression_) += constant;
  }
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

void GurobiExpression::add(
    std::shared_ptr<const ExpressionImpl> expression) try {
  auto& other = dynamic_cast<const GurobiExpression&>(*expression).get();
  if (std::holds_alternative<GRBLinExpr>(expression_)) {
    if (std::holds_alternative<GRBLinExpr>(other)) {
      std::get<GRBLinExpr>(expression_) += std::get<GRBLinExpr>(other);
    } else {
      expression_ =
          std::get<GRBLinExpr>(expression_) + std::get<GRBQuadExpr>(other);
    }
  } else {
    if (std::holds_alternative<GRBLinExpr>(other)) {
      std::get<GRBQuadExpr>(expression_) += std::get<GRBLinExpr>(other);
    } else {
      std::get<GRBQuadExpr>(expression_) += std::get<GRBQuadExpr>(other);
    }
  }
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

void GurobiExpression::multiply(double constant) try {
  if (std::holds_alternative<GRBLinExpr>(expression_)) {
    std::get<GRBLinExpr>(expression_) *= constant;
  } else {
    std::get<GRBQuadExpr>(expression_) *= constant;
  }
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

void GurobiExpression::multiply(
    std::shared_ptr<const ExpressionImpl> expression) try {
  auto& other = dynamic_cast<const GurobiExpression&>(*expression).get();
  if (std::holds_alternative<GRBQuadExpr>(expression_)) {
    throw SolverError("non-quadratic expression");
  }
  if (std::holds_alternative<GRBQuadExpr>(other)) {
    throw SolverError("non-quadratic expression");
  }
  expression_ = std::get<GRBLinExpr>(expression_) * std::get<GRBLinExpr>(other);
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

std::shared_ptr<RelationImpl> GurobiExpression::makeEqualZeroRelation() const
    try {
  const bool isLinear = std::holds_alternative<GRBLinExpr>(expression_);

  return std::make_shared<GurobiRelation>(
      isLinear ? std::get<GRBLinExpr>(expression_) == 0
               : std::get<GRBQuadExpr>(expression_) == 0,
      isLinear);
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

std::shared_ptr<RelationImpl> GurobiExpression::makeLessEqualZeroRelation()
    const try {
  const bool isLinear = std::holds_alternative<GRBLinExpr>(expression_);

  return std::make_shared<GurobiRelation>(
      isLinear ? std::get<GRBLinExpr>(expression_) <= 0
               : std::get<GRBQuadExpr>(expression_) <= 0,
      isLinear);
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

std::shared_ptr<RelationImpl> GurobiExpression::makeGreaterEqualZeroRelation()
    const try {
  const bool isLinear = std::holds_alternative<GRBLinExpr>(expression_);

  return std::make_shared<GurobiRelation>(
      isLinear ? std::get<GRBLinExpr>(expression_) >= 0
               : std::get<GRBQuadExpr>(expression_) >= 0,
      isLinear);
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

double GurobiExpression::getValue() const try {
  if (std::holds_alternative<GRBLinExpr>(expression_)) {
    return std::get<GRBLinExpr>(expression_).getValue();
  }
  return std::get<GRBQuadExpr>(expression_).getValue();
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

double GurobiExpression::computeValue() const try {
  auto getVarValue = [](const auto& var) {
    auto varType = var.get(GRB_CharAttr_VType);
    const bool isBinaryOrInteger =
        (varType == GRB_INTEGER || varType == GRB_BINARY);
    const auto gurobiValue = var.get(GRB_DoubleAttr::GRB_DoubleAttr_X);
    return isBinaryOrInteger ? round(gurobiValue) : gurobiValue;
  };

  auto getLinExprValue = [&](const auto& linExpr) {
    double exprValue = linExpr.getConstant();
    for (const auto i : folly::irange(linExpr.size())) {
      const auto& var = linExpr.getVar(i);
      exprValue += linExpr.getCoeff(i) * getVarValue(var);
    }
    return exprValue;
  };

  if (std::holds_alternative<GRBLinExpr>(expression_)) {
    return getLinExprValue(std::get<GRBLinExpr>(expression_));
  }

  // quadratic expr
  const auto& quadExpr = std::get<GRBQuadExpr>(expression_);
  double exprValue = getLinExprValue(quadExpr.getLinExpr());
  for (const auto i : folly::irange(quadExpr.size())) {
    auto var1Value = getVarValue(quadExpr.getVar1(i));
    auto var2Value = getVarValue(quadExpr.getVar2(i));
    exprValue += quadExpr.getCoeff(i) * var1Value * var2Value;
  }

  return exprValue;
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

double GurobiExpression::getConstant() const try {
  if (std::holds_alternative<GRBLinExpr>(expression_)) {
    return std::get<GRBLinExpr>(expression_).getConstant();
  }
  return std::get<GRBQuadExpr>(expression_).getLinExpr().getConstant();
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

void GurobiExpression::print() const try {
  if (std::holds_alternative<GRBLinExpr>(expression_)) {
    std::cout << std::get<GRBLinExpr>(expression_) << std::endl;
  } else {
    std::cout << std::get<GRBQuadExpr>(expression_) << std::endl;
  }
} catch (const GRBException& e) {
  rethrowGurobiError(e);
}

const std::variant<GRBLinExpr, GRBQuadExpr>& GurobiExpression::get() const {
  return expression_;
}

} // namespace facebook::algopt::lp::detail

#endif
