/*
 *  Copyright (c) 2021 NetEase Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


/*
 * Project: curve
 * Created Date: Thur May 27 2021
 * Author: xuchaojie
 */

#ifndef CURVEFS_SRC_CLIENT_FUSE_S3_CLIENT_H_
#define CURVEFS_SRC_CLIENT_FUSE_S3_CLIENT_H_

#include <memory>
#include <string>
#include <list>
#include <vector>
#include <utility>

#include "curvefs/src/client/fuse_client.h"
#include "curvefs/src/client/s3/client_s3_cache_manager.h"
#include "curvefs/src/client/warmup/warmup_manager.h"
#include "curvefs/src/volume/common.h"
#include "src/common/s3_adapter.h"

namespace curvefs {
namespace client {

using curve::common::GetObjectAsyncContext;
using curve::common::GetObjectAsyncCallBack;
using curvefs::volume::kMiB;
namespace warmup {
class WarmupManager;
class WarmupManagerS3Impl;
}  // namespace warmup


class FuseS3Client : public FuseClient {
 public:
    FuseS3Client()
        : FuseClient(), s3Adaptor_(std::make_shared<S3ClientAdaptorImpl>()) {
        auto readFunc = [this](fuse_req_t req, fuse_ino_t ino, size_t size,
                               off_t off, struct fuse_file_info* fi,
                               char* buffer, size_t* rSize) {
            return FuseOpRead(req, ino, size, off, fi, buffer, rSize);
        };
        auto readLinkFunc = [this](fuse_req_t req, fuse_ino_t ino,
                                   std::string* linkStr) {
            return FuseClient::FuseOpReadLink(req, ino, linkStr);
        };
        warmupManager_ = std::make_shared<warmup::WarmupManagerS3Impl>(
            metaClient_, inodeManager_, dentryManager_, fsInfo_, readFunc,
            readLinkFunc, nullptr, s3Adaptor_);
    }

    FuseS3Client(const std::shared_ptr<MdsClient> &mdsClient,
                 const std::shared_ptr<MetaServerClient> &metaClient,
                 const std::shared_ptr<InodeCacheManager> &inodeManager,
                 const std::shared_ptr<DentryCacheManager> &dentryManager,
                 const std::shared_ptr<S3ClientAdaptor> &s3Adaptor,
                 const std::shared_ptr<warmup::WarmupManager> &warmupManager)
        : FuseClient(mdsClient, metaClient, inodeManager, dentryManager,
                     warmupManager),
          s3Adaptor_(s3Adaptor) {}

    CURVEFS_ERROR Init(const FuseClientOption &option) override;

    void UnInit() override;

    CURVEFS_ERROR FuseOpInit(
        void *userdata, struct fuse_conn_info *conn) override;

    CURVEFS_ERROR FuseOpWrite(fuse_req_t req, fuse_ino_t ino,
        const char *buf, size_t size, off_t off,
        struct fuse_file_info *fi, FileOut* fileOut) override;

    CURVEFS_ERROR FuseOpRead(fuse_req_t req,
        fuse_ino_t ino, size_t size, off_t off,
        struct fuse_file_info *fi,
        char *buffer,
        size_t *rSize) override;

    CURVEFS_ERROR FuseOpCreate(fuse_req_t req,
                               fuse_ino_t parent,
                               const char* name,
                               mode_t mode,
                               struct fuse_file_info *fi,
                               EntryOut* entryOut) override;

    CURVEFS_ERROR FuseOpMkNod(fuse_req_t req,
                              fuse_ino_t parent,
                              const char* name,
                              mode_t mode,
                              dev_t rdev,
                              EntryOut* entryOut) override;

    CURVEFS_ERROR FuseOpLink(fuse_req_t req,
                             fuse_ino_t ino,
                             fuse_ino_t newparent,
                             const char *newname,
                             EntryOut* entryOut) override;

    CURVEFS_ERROR FuseOpUnlink(fuse_req_t req, fuse_ino_t parent,
        const char *name) override;

    CURVEFS_ERROR FuseOpFsync(fuse_req_t req, fuse_ino_t ino, int datasync,
        struct fuse_file_info *fi) override;

    CURVEFS_ERROR FuseOpFlush(fuse_req_t req, fuse_ino_t ino,
        struct fuse_file_info *fi) override;

    CURVEFS_ERROR Truncate(InodeWrapper *inode, uint64_t length) override;

 private:
    bool InitKVCache(const KVClientManagerOpt &opt);

    void FlushData() override;

 private:
    // s3 adaptor
    std::shared_ptr<S3ClientAdaptor> s3Adaptor_;
    std::shared_ptr<KVClientManager> kvClientManager_;

    static constexpr auto MIN_WRITE_CACHE_SIZE = 8 * kMiB;
};


}  // namespace client
}  // namespace curvefs

#endif  // CURVEFS_SRC_CLIENT_FUSE_S3_CLIENT_H_
