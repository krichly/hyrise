#pragma once

#include <string>
#include <vector>

#include "abstract_lqp_node.hpp"
#include "column_origin.hpp"
#include "types.hpp"

namespace opossum {

/**
 * Struct to specify Order By items.
 * Order By items are defined by the column they operate on and their sort order.
 */
struct OrderByDefinition {
  OrderByDefinition(const ColumnOrigin& column_origin, const OrderByMode order_by_mode);

  ColumnOrigin column_origin;
  OrderByMode order_by_mode;
};

using OrderByDefinitions = std::vector<OrderByDefinition>;

/**
 * This node type represents sorting operations as defined in ORDER BY clauses.
 */
class SortNode : public AbstractLQPNode {
 public:
  explicit SortNode(const OrderByDefinitions& order_by_definitions);

  std::string description() const override;

  const OrderByDefinitions& order_by_definitions() const;

 protected:
  std::shared_ptr<AbstractLQPNode> _deep_copy_impl(const std::shared_ptr<AbstractLQPNode>& left_child, const std::shared_ptr<AbstractLQPNode>& right_child) const override;

 private:
  const OrderByDefinitions _order_by_definitions;
};

}  // namespace opossum
