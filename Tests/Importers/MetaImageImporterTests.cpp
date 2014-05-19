#include "catch.hpp"
#include "MetaImageImporter.hpp"
#include "DeviceManager.hpp"

using namespace fast;

TEST_CASE("No filename set to MetaImageImporter", "[fast][MetaImageImporter]") {
    MetaImageImporter::pointer importer = MetaImageImporter::New();
    CHECK_THROWS(importer->update());
}

TEST_CASE("Import non-existing file to MetaImageImporter", "[fast][MetaImageImporter]") {
    MetaImageImporter::pointer importer = MetaImageImporter::New();
    importer->setFilename("asdasdasdsad");
    CHECK_THROWS_AS(importer->update(), FileNotFoundException);
}

TEST_CASE("Import MetaImage file to host", "[fast][MetaImageImporter]") {
    MetaImageImporter::pointer importer = MetaImageImporter::New();
    importer->setFilename(std::string(FAST_TEST_DATA_DIR)+"US-3Dt/US-3Dt_0.mhd");
    importer->setDevice(Host::New());
    Image::pointer image = importer->getOutput();
    image->update();

    // Check attributes of image
    CHECK(image->getWidth() == 276);
    CHECK(image->getHeight() == 249);
    CHECK(image->getDepth() == 200);
    CHECK(image->getDimensions() == 3);
    CHECK(image->getSpacing().x() == Approx(0.309894));
    CHECK(image->getSpacing().y() == Approx(0.241966));
    CHECK(image->getSpacing().z() == Approx(0.430351));
    CHECK(image->getOffset().x() == Approx(-20.2471));
    CHECK(image->getOffset().y() == Approx(-191.238));
    CHECK(image->getOffset().z() == Approx(-65.9711));
    CHECK(image->getCenterOfRotation().x() == Approx(0));
    CHECK(image->getCenterOfRotation().y() == Approx(0));
    CHECK(image->getCenterOfRotation().z() == Approx(0));
    CHECK(image->getTransformMatrix3D()[0] == Approx(0.0784201));
    CHECK(image->getTransformMatrix3D()[1] == Approx(0.0356554));
    CHECK(image->getTransformMatrix3D()[2] == Approx(-0.996283));
    CHECK(image->getTransformMatrix3D()[3] == Approx(-0.0697932));
    CHECK(image->getTransformMatrix3D()[4] == Approx(0.997105));
    CHECK(image->getTransformMatrix3D()[5] == Approx(0.0301913));
    CHECK(image->getTransformMatrix3D()[6] == Approx(0.994474));
    CHECK(image->getTransformMatrix3D()[7] == Approx(0.0671661));
    CHECK(image->getTransformMatrix3D()[8] == Approx(0.0806815));
    CHECK(image->getDataType() == TYPE_UINT8);
}

TEST_CASE("Import MetaImage file to OpenCL device", "[fast][MetaImageImporter]") {
    DeviceManager& deviceManager = DeviceManager::getInstance();
    OpenCLDevice::pointer device = deviceManager.getOneOpenCLDevice();

    MetaImageImporter::pointer importer = MetaImageImporter::New();
    importer->setFilename(std::string(FAST_TEST_DATA_DIR)+"US-3Dt/US-3Dt_0.mhd");
    importer->setDevice(device);
    Image::pointer image = importer->getOutput();
    image->update();

    // Check attributes of image
    CHECK(image->getWidth() == 276);
    CHECK(image->getHeight() == 249);
    CHECK(image->getDepth() == 200);
    CHECK(image->getSpacing().x() == Approx(0.309894));
    CHECK(image->getSpacing().y() == Approx(0.241966));
    CHECK(image->getSpacing().z() == Approx(0.430351));
    CHECK(image->getDimensions() == 3);
    CHECK(image->getDataType() == TYPE_UINT8);
}

