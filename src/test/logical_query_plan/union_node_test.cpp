#include <memory>

#include "gtest/gtest.h"

#include "base_test.hpp"

#include "logical_query_plan/mock_node.cpp"
#include "logical_query_plan/union_node.hpp"

namespace opossum {

class UnionNodeTest : public BaseTest {
 protected:
  void SetUp() override {
    _mock_node = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "a"}, {DataType::Int, "b"}, {DataType::Int, "c"}}, "t_a");

    _a = {_mock_node, ColumnID{0}};
    _b = {_mock_node, ColumnID{1}};
    _c = {_mock_node, ColumnID{2}};

    _union_node = std::make_shared<UnionNode>(UnionMode::Positions);
    _union_node->set_left_child(_mock_node);
    _union_node->set_right_child(_mock_node);
  }

  std::shared_ptr<MockNode> _mock_node;
  std::shared_ptr<UnionNode> _union_node;
  LQPColumnOrigin _a;
  LQPColumnOrigin _b;
  LQPColumnOrigin _c;
};

TEST_F(UnionNodeTest, Description) { EXPECT_EQ(_union_node->description(), "[UnionNode] Mode: UnionPositions"); }

TEST_F(UnionNodeTest, StatisticsNotImplemented) {
  EXPECT_THROW(_union_node->derive_statistics_from(_mock_node, _mock_node), std::exception);
}

TEST_F(UnionNodeTest, ColumnOriginByNamedColumnReference) {
  EXPECT_EQ(_union_node->get_column_origin_by_named_column_reference({"a"}), _a);
  EXPECT_EQ(_union_node->get_column_origin_by_named_column_reference({"b"}), _b);
  EXPECT_EQ(_union_node->get_column_origin_by_named_column_reference({"c"}), _c);
}

TEST_F(UnionNodeTest, OutputColumnOrigins) {
  EXPECT_EQ(_union_node->output_column_origins().at(0), _a);
  EXPECT_EQ(_union_node->output_column_origins().at(1), _b);
  EXPECT_EQ(_union_node->output_column_origins().at(2), _c);
}

TEST_F(UnionNodeTest, MismatchingColumnNames) {
  /**
   * If the input tables have different column layouts get_verbose_column_name() will fail
   */
  auto mock_node_b = std::make_shared<MockNode>(MockNode::ColumnDefinitions{{DataType::Int, "a"}, {DataType::Int, "d"}, {DataType::Int, "c"}}, "t_a");

  auto invalid_union = std::make_shared<UnionNode>(UnionMode::Positions);
  invalid_union->set_left_child(_mock_node);
  invalid_union->set_right_child(mock_node_b);

  EXPECT_THROW(invalid_union->get_verbose_column_name(ColumnID{1}), std::exception);
}

TEST_F(UnionNodeTest, VerboseColumnNames) {
  /**
   * UnionNode will only prefix columns with its own ALIAS and forget any table names / aliases of its input tables
   */
  auto verbose_union = std::make_shared<UnionNode>(UnionMode::Positions);
  verbose_union->set_left_child(_mock_node);
  verbose_union->set_right_child(_mock_node);
  verbose_union->set_alias("union_alias");

  EXPECT_EQ(_union_node->get_verbose_column_name(ColumnID{0}), "a");
  EXPECT_EQ(_union_node->get_verbose_column_name(ColumnID{1}), "b");
  EXPECT_EQ(verbose_union->get_verbose_column_name(ColumnID{0}), "union_alias.a");
  EXPECT_EQ(verbose_union->get_verbose_column_name(ColumnID{1}), "union_alias.b");
}
//
//TEST_F(UnionNodeTest, FindingColumnReferences) {
//  /**
//    * UnionNode can only resolve column references without a table_name
//    */
//
//  // finding column named "a"
//  EXPECT_EQ(_union_node->get_column_id_by_named_column_reference({"a", std::nullopt}), ColumnID{0});
//  EXPECT_EQ(_union_node->find_column_id_by_named_column_reference({"a", {"table_a"}}), std::nullopt);
//  EXPECT_EQ(_union_node->find_column_id_by_named_column_reference({"a", {"table_b"}}), std::nullopt);
//
//  // finding column named "b"
//  EXPECT_EQ(_union_node->get_column_id_by_named_column_reference({"b", std::nullopt}), ColumnID{1});
//  EXPECT_EQ(_union_node->find_column_id_by_named_column_reference({"b", {"table_a"}}), std::nullopt);
//  EXPECT_EQ(_union_node->find_column_id_by_named_column_reference({"b", {"table_b"}}), std::nullopt);
//
//  // invalid names
//  EXPECT_EQ(_union_node->find_column_id_by_named_column_reference({"c", std::nullopt}), std::nullopt);
//  EXPECT_EQ(_union_node->find_column_id_by_named_column_reference({"c", {"table_a"}}), std::nullopt);
//  EXPECT_EQ(_union_node->find_column_id_by_named_column_reference({"c", {"table_b"}}), std::nullopt);
//}

}  // namespace opossum
