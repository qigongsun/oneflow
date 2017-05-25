#include "graph/model_update_comp_task_node.h"
#include "graph/model_update_task_graph.h"

namespace oneflow {

void MdUpdtCompTaskNode::BuildExecAndEnrollLbn2Regsts(TaskGraph* gph) {
  CHECK(IsFwNode());
  auto md_updt_gph = of_dynamic_cast<MdUpdtTaskGraph*> (gph);
  CompTaskNode* fw_task = md_updt_gph->GetFwTaskFromParallelId(parallel_id());
  TaskNode* bp_task = fw_task->GetBpNode();
  std::shared_ptr<RegstDesc> model_diff_regst;
  if (bp_task != nullptr) {
    model_diff_regst = bp_task->GetProducedRegstDesc("model_diff");
  }
  if (chain_node()->op_vec().empty()) {
    BindProducedRegstAndOutEdge(model_diff_regst, SoleOutEdge());
    return;
  }
  TakeOverRegstDesc(fw_task, "model");
  TakeOverRegstDesc(fw_task, "model_tmp");
  auto model_regst = GetProducedRegstDesc("model");
  if (chain_node()->parallel_desc()->device_type() == kGPU) {
    auto model_load_buf_regst = NewProducedRegstDesc("model_load_buf");
    model_load_buf_regst->CopyLbnFrom(model_regst.get());
  } else {
    CHECK(chain_node()->parallel_desc()->device_type() == kCPU);
  }

  ExecNode* exec_node = mut_exec_gph().NewNode();
  exec_node->mut_op() = chain_node()->SoleOp();
  const std::string ibn = "model_diffs";
  if (in_edges().empty()) {
    if (model_diff_regst) {
      exec_node->BindBnInOpAndRegst(ibn, model_diff_regst);
      SubscribeRegstDesc(ibn, model_diff_regst);
    }
  } else {
    exec_node->BindBnInOpAndRegst(ibn, GetRelatedRegst(SoleInEdge()));
    SubscribeRegstDesc(ibn, GetRelatedRegst(SoleInEdge()));
  }
  exec_node->BindBnInOpAndRegst(exec_node->op()->SoleObn(), model_regst);
  mut_exec_gph().UpdateSourceAndSink();
}

void MdUpdtCompTaskNode::InferShapeOfBlobsInProducedRegsts(TaskGraph* gph) {
  CHECK(IsFwNode());
  if (auto model_load_buf_regst = GetProducedRegstDesc("model_load_buf")) {
    auto model_regst = GetProducedRegstDesc("model");
    model_load_buf_regst->CopyShapeFrom(model_regst.get());
  }
}

} // namespace oneflow
