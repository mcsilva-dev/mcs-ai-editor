#include "media/ffmpeg_info.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

namespace mcs::media {

QString ffmpegVersion() {
    return QString::fromUtf8(av_version_info());
}

bool hasDecoder(const char* name) {
    return avcodec_find_decoder_by_name(name) != nullptr;
}

} // namespace mcs::media
