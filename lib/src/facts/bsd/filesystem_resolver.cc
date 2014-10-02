#include <facter/facts/bsd/filesystem_resolver.hpp>
#include <facter/logging/logging.hpp>
#include <facter/util/string.hpp>
#include <sys/mount.h>
#include <tuple>

using namespace std;
using namespace facter::facts;
using namespace facter::util;

LOG_DECLARE_NAMESPACE("facts.bsd.filesystem");

namespace facter { namespace facts { namespace bsd {

    filesystem_resolver::data filesystem_resolver::collect_data(collection& facts)
    {
        data result;

        // First get the count of file systems
        int count = getfsstat(nullptr, 0, MNT_NOWAIT);
        if (count == -1) {
            LOG_ERROR("getfsstat failed: %1% (%2%): file system facts are unavailable.", strerror(errno), errno);
            return result;
        }

        // Get the actual data
        vector<struct statfs> filesystems(count);
        count = getfsstat(filesystems.data(), filesystems.size() * sizeof(struct statfs), MNT_NOWAIT);
        if (count == -1) {
            LOG_ERROR("getfsstat failed: %1% (%2%): file system facts are unavailable.", strerror(errno), errno);
            return result;
        }

        result.mountpoints.reserve(count);

        // Populate an entry for each mounted file system
        for (auto& fs : filesystems) {
            mountpoint point;
            point.name = fs.f_mntonname;
            point.device = fs.f_mntfromname;
            point.filesystem = fs.f_fstypename;
            point.size = fs.f_bsize * fs.f_blocks;
            point.available = fs.f_bsize * fs.f_bfree;
            point.options = to_options(fs);
            result.mountpoints.emplace_back(move(point));

            result.filesystems.insert(fs.f_fstypename);
        }
        return result;
    }

    vector<string> filesystem_resolver::to_options(struct statfs const& fs)
    {
        static vector<tuple<unsigned int, string>> const flags = {
            { MNT_RDONLY,       "readonly" },
            { MNT_SYNCHRONOUS,  "noasync" },
            { MNT_NOEXEC,       "noexec" },
            { MNT_NOSUID,       "nosuid" },
            { MNT_NODEV,        "nodev" },
            { MNT_UNION,        "union" },
            { MNT_ASYNC,        "async" },
            { MNT_EXPORTED,     "exported" },
            { MNT_LOCAL,        "local" },
            { MNT_QUOTA,        "quota" },
            { MNT_ROOTFS,       "root" },
            { MNT_DONTBROWSE,   "nobrowse" },
            { MNT_AUTOMOUNTED,  "automounted" },
            { MNT_JOURNALED,    "journaled" },
            { MNT_DEFWRITE,     "deferwrites" },
        };

        vector<string> options;
        for (auto const& flag : flags) {
            if (fs.f_flags & get<0>(flag)) {
                options.push_back(get<1>(flag));
            }
        }
        return options;
    }

}}}  // namespace facter::facts::bsd
