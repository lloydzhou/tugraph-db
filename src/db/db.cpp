﻿/* Copyright (c) 2022 AntGroup. All Rights Reserved. */

#include "db/db.h"

lgraph::AccessControlledDB::AccessControlledDB(ScopedRef<LightningGraph>&& ref,
                                               AccessLevel access_level)
    : graph_ref_(std::move(ref)),
      graph_(graph_ref_.Get()),
      graph_ref_lock_(graph_->GetReloadLock(), GetMyThreadId()),
      access_level_(access_level) {}

lgraph::AccessControlledDB::AccessControlledDB(lgraph::LightningGraph* db)
    : graph_ref_(nullptr, GetMyThreadId()),
      graph_(db),
      graph_ref_lock_(db->GetReloadLock(), GetMyThreadId()),
      access_level_(AccessLevel::FULL) {}

lgraph::AccessControlledDB::AccessControlledDB(AccessControlledDB&& rhs)
    : graph_ref_(std::move(rhs.graph_ref_)),
      graph_(rhs.graph_),
      graph_ref_lock_(graph_->GetReloadLock(), GetMyThreadId()),
      access_level_(rhs.access_level_) {}

const lgraph::DBConfig& lgraph::AccessControlledDB::GetConfig() const {
    CheckReadAccess();
    return graph_->GetConfig();
}

lgraph::Transaction lgraph::AccessControlledDB::CreateReadTxn() {
    CheckReadAccess();
    return graph_->CreateReadTxn();
}

lgraph::Transaction lgraph::AccessControlledDB::CreateWriteTxn(bool optimistic, bool flush) {
    CheckWriteAccess();
    return graph_->CreateWriteTxn(optimistic, flush);
}

lgraph::Transaction lgraph::AccessControlledDB::ForkTxn(Transaction& txn) {
    return graph_->ForkTxn(txn);
}

bool lgraph::AccessControlledDB::LoadPlugin(plugin::Type plugin_type, const std::string& token,
                                            const std::string& name, const std::string& code,
                                            plugin::CodeType code_type, const std::string& desc,
                                            bool is_read_only) {
    CheckFullAccess();
    return graph_->GetPluginManager()->LoadPluginFromCode(plugin_type, token, name, code, code_type,
                                                          desc, is_read_only);
}

bool lgraph::AccessControlledDB::DelPlugin(plugin::Type plugin_type, const std::string& token,
                                           const std::string& name) {
    CheckFullAccess();
    return graph_->GetPluginManager()->DelPlugin(plugin_type, token, name);
}

bool lgraph::AccessControlledDB::CallPlugin(plugin::Type plugin_type, const std::string& token,
                                            const std::string& name, const std::string& request,
                                            double timeout_seconds, bool in_process,
                                            std::string& output) {
    auto pm = graph_->GetPluginManager();
    int r = pm->IsReadOnlyPlugin(plugin_type, token, name);
    if (r < 0) return false;
    bool read_only = (bool)r;
    if (access_level_ < AccessLevel::WRITE && !read_only)
        throw AuthError("Write permission needed to call this plugin.");
    return pm->Call(plugin_type, token, this, name, request, timeout_seconds, in_process, output);
}

std::vector<lgraph::PluginDesc> lgraph::AccessControlledDB::ListPlugins(plugin::Type plugin_type,
                                                                        const std::string& token) {
    return graph_->GetPluginManager()->ListPlugins(plugin_type, token);
}

bool lgraph::AccessControlledDB::GetPluginCode(plugin::Type plugin_type, const std::string& token,
                                               const std::string& name, lgraph::PluginCode& ret) {
    return graph_->GetPluginManager()->GetPluginCode(plugin_type, token, name, ret);
}

bool lgraph::AccessControlledDB::IsReadOnlyPlugin(plugin::Type type, const std::string& token,
                                                  const std::string& name, bool& is_readonly) {
    int r = graph_->GetPluginManager()->IsReadOnlyPlugin(type, token, name);
    if (r < 0) return false;
    is_readonly = (bool)r;
    return true;
}

void lgraph::AccessControlledDB::DropAllData() {
    CheckFullAccess();
    graph_->DropAllData();
}

void lgraph::AccessControlledDB::DropAllVertex() {
    CheckFullAccess();
    graph_->DropAllVertex();
}

void lgraph::AccessControlledDB::Flush() {
    CheckWriteAccess();
    graph_->Persist();
}

size_t lgraph::AccessControlledDB::EstimateNumVertices() {
    CheckReadAccess();
    return graph_->GetNumVertices();
}

bool lgraph::AccessControlledDB::AddLabel(bool is_vertex, const std::string& label,
                                          const std::vector<FieldSpec>& fds,
                                          const std::string& primary_field,
                                          const EdgeConstraints& edge_constraints) {
    CheckFullAccess();
    return graph_->AddLabel(label, fds, is_vertex, primary_field, edge_constraints);
}

bool lgraph::AccessControlledDB::DeleteLabel(bool is_vertex, const std::string& label,
                                             size_t* n_modified) {
    CheckFullAccess();
    return graph_->DelLabel(label, is_vertex, n_modified);
}

bool lgraph::AccessControlledDB::AlterLabelModEdgeConstraints(
    const std::string& label, const std::vector<std::pair<std::string, std::string>>& constraints) {
    CheckFullAccess();
    return graph_->AlterLabelModEdgeConstraints(label, constraints);
}

bool lgraph::AccessControlledDB::AlterLabelDelFields(const std::string& label,
                                                     const std::vector<std::string>& del_fields,
                                                     bool is_vertex, size_t* n_modified) {
    CheckFullAccess();
    return graph_->AlterLabelDelFields(label, del_fields, is_vertex, n_modified);
}

bool lgraph::AccessControlledDB::AlterLabelAddFields(const std::string& label,
                                                     const std::vector<FieldSpec>& add_fields,
                                                     const std::vector<FieldData>& default_values,
                                                     bool is_vertex, size_t* n_modified) {
    CheckFullAccess();
    return graph_->AlterLabelAddFields(label, add_fields, default_values, is_vertex, n_modified);
}

bool lgraph::AccessControlledDB::AlterLabelModFields(const std::string& label,
                                                     const std::vector<FieldSpec>& mod_fields,
                                                     bool is_vertex, size_t* n_modified) {
    CheckFullAccess();
    return graph_->AlterLabelModFields(label, mod_fields, is_vertex, n_modified);
}

bool lgraph::AccessControlledDB::AddVertexIndex(const std::string& label, const std::string& field,
                                                bool is_unique) {
    CheckFullAccess();
    return graph_->BlockingAddIndex(label, field, is_unique, true);
}

bool lgraph::AccessControlledDB::AddEdgeIndex(const std::string& label, const std::string& field,
                                          bool is_unique) {
    CheckFullAccess();
    return graph_->BlockingAddIndex(label, field, is_unique, false);
}

bool lgraph::AccessControlledDB::AddFullTextIndex(bool is_vertex, const std::string& label,
                                                  const std::string& field) {
    CheckFullAccess();
    return graph_->AddFullTextIndex(is_vertex, label, field);
}

bool lgraph::AccessControlledDB::DeleteFullTextIndex(bool is_vertex, const std::string& label,
                                                     const std::string& field) {
    CheckFullAccess();
    return graph_->DeleteFullTextIndex(is_vertex, label, field);
}

void lgraph::AccessControlledDB::RebuildFullTextIndex(const std::set<std::string>& vertex_labels,
                                                      const std::set<std::string>& edge_labels) {
    CheckFullAccess();
    return graph_->RebuildFullTextIndex(vertex_labels, edge_labels);
}

std::vector<std::tuple<bool, std::string, std::string>>
lgraph::AccessControlledDB::ListFullTextIndexes() {
    CheckFullAccess();
    return graph_->ListFullTextIndexes();
}

std::vector<std::pair<int64_t, float>> lgraph::AccessControlledDB::QueryVertexByFullTextIndex(
    const std::string& label, const std::string& query, int top_n) {
    CheckFullAccess();
    return graph_->QueryVertexByFullTextIndex(label, query, top_n);
}

std::vector<std::pair<lgraph_api::EdgeUid, float>>
lgraph::AccessControlledDB::QueryEdgeByFullTextIndex(const std::string& label,
                                                     const std::string& query, int top_n) {
    CheckFullAccess();
    return graph_->QueryEdgeByFullTextIndex(label, query, top_n);
}

bool lgraph::AccessControlledDB::DeleteVertexIndex(const std::string& label,
                                                   const std::string& field) {
    CheckFullAccess();
    return graph_->DeleteIndex(label, field, true);
}

bool lgraph::AccessControlledDB::DeleteEdgeIndex(const std::string& label,
                                                 const std::string& field) {
    CheckFullAccess();
    return graph_->DeleteIndex(label, field, false);
}

bool lgraph::AccessControlledDB::IsVertexIndexed(const std::string& label,
                                                 const std::string& field) {
    CheckReadAccess();
    return graph_->IsIndexed(label, field, true);
}

bool lgraph::AccessControlledDB::IsEdgeIndexed(const std::string& label,
                                               const std::string& field) {
    CheckReadAccess();
    return graph_->IsIndexed(label, field, false);
}

void lgraph::AccessControlledDB::WarmUp() const { graph_->WarmUp(); }

size_t lgraph::AccessControlledDB::Backup(const std::string& path, bool compact) const {
    CheckReadAccess();
    return graph_->Backup(path, compact);
}