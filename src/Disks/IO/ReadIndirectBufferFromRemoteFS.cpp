#include "ReadIndirectBufferFromRemoteFS.h"

#include <Disks/IO/ReadBufferFromRemoteFSGather.h>


namespace DB
{

namespace ErrorCodes
{
    extern const int CANNOT_SEEK_THROUGH_FILE;
}


ReadIndirectBufferFromRemoteFS::ReadIndirectBufferFromRemoteFS(
    std::shared_ptr<ReadBufferFromRemoteFSGather> impl_) : impl(std::move(impl_))
{
}


off_t ReadIndirectBufferFromRemoteFS::getPosition()
{
    return impl->absolute_position - available();
}


String ReadIndirectBufferFromRemoteFS::getFileName() const
{
    return impl->getFileName();
}


off_t ReadIndirectBufferFromRemoteFS::seek(off_t offset_, int whence)
{
    if (whence == SEEK_CUR)
    {
        /// If position within current working buffer - shift pos.
        if (!working_buffer.empty() && size_t(getPosition() + offset_) < impl->absolute_position)
        {
            pos += offset_;
            return getPosition();
        }
        else
        {
            impl->absolute_position += offset_;
        }
    }
    else if (whence == SEEK_SET)
    {
        /// If position within current working buffer - shift pos.
        if (!working_buffer.empty()
            && size_t(offset_) >= impl->absolute_position - working_buffer.size()
            && size_t(offset_) < impl->absolute_position)
        {
            pos = working_buffer.end() - (impl->absolute_position - offset_);
            return getPosition();
        }
        else
        {
            impl->absolute_position = offset_;
        }
    }
    else
        throw Exception("Only SEEK_SET or SEEK_CUR modes are allowed.", ErrorCodes::CANNOT_SEEK_THROUGH_FILE);

    impl->reset();
    pos = working_buffer.end();

    return impl->absolute_position;
}


bool ReadIndirectBufferFromRemoteFS::nextImpl()
{
    /// Transfer current position and working_buffer to actual ReadBuffer
    swap(*impl);
    /// Position and working_buffer will be updated in next() call
    auto result = impl->next();
    /// and assigned to current buffer.
    swap(*impl);

    return result;
}

}
