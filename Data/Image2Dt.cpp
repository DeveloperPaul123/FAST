#include "Image2Dt.hpp"
using namespace fast;

Image::pointer Image2Dt::getNextFrame() {
    mStreamMutex.lock();
    // TODO: If no frame is available the method should maybe wait?
    if(mFrames.size() == 0)
        throw Exception("No frames available");
    Image::pointer ret = mFrames[mCurrentFrame];
    mStreamMutex.unlock();
    if(mKeepAllFrames)
        mCurrentFrame++;
    return ret;
}

void Image2Dt::addFrame(Image::pointer frame) {
    mStreamMutex.lock();
    updateModifiedTimestamp();
    if(!mKeepAllFrames) {
        mFrames.clear();
    }
    mFrames.push_back(frame);
    mStreamMutex.unlock();
}

Image2Dt::Image2Dt() {
    mCurrentFrame = 0;
    mKeepAllFrames = false;
    mIsDynamicData = true;
    updateModifiedTimestamp();
}


unsigned int Image2Dt::getSize() const {
    return mFrames.size();
}
