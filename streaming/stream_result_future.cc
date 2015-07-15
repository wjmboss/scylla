/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * Modified by Cloudius Systems.
 * Copyright 2015 Cloudius Systems.
 */

#include "streaming/stream_result_future.hh"
#include "streaming/stream_manager.hh"
#include "log.hh"

namespace streaming {

extern logging::logger sslog;

void stream_result_future::init(UUID plan_id_, sstring description_, std::vector<stream_event_handler*> listeners_, shared_ptr<stream_coordinator> coordinator_) {
    auto future = create_and_register(plan_id_, description_, coordinator_);
    for (auto& listener : listeners_) {
        future->add_event_listener(listener);
    }

    sslog.info("[Stream #{}] Executing streaming plan for {}", plan_id_,  description_);

    // Initialize and start all sessions
    for (auto& session : coordinator_->get_all_stream_sessions()) {
        session->init(future);
    }
    coordinator_->connect_all_stream_sessions();
}

void stream_result_future::init_receiving_side(int session_index, UUID plan_id,
    sstring description, inet_address from, bool keep_ss_table_level) {
    auto& sm = get_local_stream_manager();
    auto f = sm.get_receiving_stream(plan_id);
    if (f == nullptr) {
        sslog.info("[Stream #{} ID#{}] Creating new streaming plan for {}", plan_id, session_index, description);
        // The main reason we create a StreamResultFuture on the receiving side is for JMX exposure.
        // TODO: stream_result_future needs a ref to stream_coordinator.
        sm.register_receiving(make_shared<stream_result_future>(plan_id, description, keep_ss_table_level));
    }
    sslog.info("[Stream #{}, ID#{}] Received streaming plan for {}", plan_id, session_index, description);
}

} // namespace streaming
