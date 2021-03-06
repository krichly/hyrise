#include "lqp_expression.hpp"

#include "constant_mappings.hpp"
#include "lqp_column_reference.hpp"
#include "operators/pqp_expression.hpp"
#include "utils/assert.hpp"

namespace opossum {

std::shared_ptr<LQPExpression> LQPExpression::create_column(const LQPColumnReference& column_reference,
                                                            const std::optional<std::string>& alias) {
  auto expression = std::make_shared<LQPExpression>(ExpressionType::Column);
  expression->_column_references = column_reference;
  expression->_alias = alias;

  return expression;
}

std::vector<std::shared_ptr<LQPExpression>> LQPExpression::create_columns(
    const std::vector<LQPColumnReference>& column_references, const std::optional<std::vector<std::string>>& aliases) {
  std::vector<std::shared_ptr<LQPExpression>> column_expressions;
  column_expressions.reserve(column_references.size());

  if (!aliases) {
    for (const auto& column_reference : column_references) {
      column_expressions.emplace_back(create_column(column_reference));
    }
  } else {
    DebugAssert(column_references.size() == (*aliases).size(), "There must be the same number of aliases as ColumnIDs");

    for (auto column_index = 0u; column_index < column_references.size(); ++column_index) {
      column_expressions.emplace_back(create_column(column_references[column_index], (*aliases)[column_index]));
    }
  }

  return column_expressions;
}

const LQPColumnReference& LQPExpression::column_reference() const {
  DebugAssert(_column_references,
              "Expression " + expression_type_to_string.at(_type) + " does not have a LQPColumnReference");
  return *_column_references;
}

void LQPExpression::set_column_reference(const LQPColumnReference& column_reference) {
  Assert(_type == ExpressionType::Column, "Can't set an LQPColumnReference on a non-column");
  _column_references = column_reference;
}

std::string LQPExpression::to_string(const std::optional<std::vector<std::string>>& input_column_names,
                                     bool is_root) const {
  if (type() == ExpressionType::Column) {
    return column_reference().description();
  }
  return AbstractExpression<LQPExpression>::to_string(input_column_names, is_root);
}

bool LQPExpression::operator==(const LQPExpression& other) const {
  if (!AbstractExpression<LQPExpression>::operator==(other)) {
    return false;
  }
  return _column_references == other._column_references;
}

void LQPExpression::_deep_copy_impl(const std::shared_ptr<LQPExpression>& copy) const {
  copy->_column_references = _column_references;
}
}  // namespace opossum
