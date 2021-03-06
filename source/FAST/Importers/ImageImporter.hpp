#ifndef ImageImporter_HPP_
#define ImageImporter_HPP_

#include "Importer.hpp"
#include <string>

namespace fast {

class ImageImporter : public Importer {
    FAST_OBJECT(ImageImporter)
    public:
        void setFilename(std::string filename);
        ~ImageImporter() {};
    private:
        ImageImporter();
        uchar* readBMPFile(std::string filename, int &width, int &height);
        std::string mFilename;
        void execute();

};


} // end namespace fast



#endif /* ImageImporter_HPP_ */
