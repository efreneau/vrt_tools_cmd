#include <gtest/gtest.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <vrt/vrt_init.h>
#include <vrt/vrt_read.h>
#include <vrt/vrt_types.h>
#include <vrt/vrt_util.h>
#include <vrt/vrt_words.h>

#include "../../src/process.h"
#include "../../src/program_arguments.h"
#include "common/byte_swap.h"
#include "common/generate_packet_sequence.h"

namespace fs = ::std::filesystem;

static const size_t   N_PACKETS{10};
static const fs::path TMP_DIR{"test_tmp"};
static const fs::path TMP_FILE_NAME{"split.vrt"};
static const fs::path TMP_FILE_PATH{TMP_DIR / TMP_FILE_NAME};

class StreamTest : public ::testing::Test {
   protected:
    StreamTest() : p_() {}

    void SetUp() override {
        try {
            fs::remove_all(TMP_DIR);
        } catch (const fs::filesystem_error&) {
            // Do nothing
        }
        fs::create_directory(TMP_DIR);
        vrt_init_packet(&p_);
    }
    void TearDown() override {
        try {
            fs::remove_all(TMP_DIR);
        } catch (const fs::filesystem_error&) {
            // Do nothing
        }
    }

    vrt_packet p_;
};

static void process() {
    vrt::length::ProgramArguments args;
    args.file_path_in = TMP_FILE_PATH;
    vrt::length::process(args);
}

TEST_F(StreamTest, SameClassId) {
    p_.header.has.class_id                    = true;
    p_.fields.class_id.oui                    = 0xDEDEDE;
    p_.fields.class_id.information_class_code = 0xBEBE;
    p_.fields.class_id.packet_class_code      = 0xABAB;
    vrt::generate_packet_sequence(TMP_FILE_PATH, &p_, N_PACKETS);

    process();
}

TEST_F(StreamTest, SameStreamId) {
    p_.header.packet_type = VRT_PT_IF_DATA_WITH_STREAM_ID;
    p_.fields.stream_id   = 0xDEADBEEF;
    vrt::generate_packet_sequence(TMP_FILE_PATH, &p_, N_PACKETS);

    process();
}

TEST_F(StreamTest, SameAll) {
    p_.header.packet_type                     = VRT_PT_IF_DATA_WITH_STREAM_ID;
    p_.fields.stream_id                       = 0xDEADBEEF;
    p_.header.has.class_id                    = true;
    p_.fields.class_id.oui                    = 0xDEDEDE;
    p_.fields.class_id.information_class_code = 0xBEBE;
    p_.fields.class_id.packet_class_code      = 0xABAB;
    vrt::generate_packet_sequence(TMP_FILE_PATH, &p_, N_PACKETS);

    process();
}

TEST_F(StreamTest, Oui) {
    p_.header.has.class_id = true;
    vrt::generate_packet_sequence(TMP_FILE_PATH, &p_, N_PACKETS,
                                  [](uint64_t i, vrt_packet* p) { p->fields.class_id.oui = i % 4; });

    process();
}

TEST_F(StreamTest, InformationClassCode) {
    p_.header.has.class_id = true;
    vrt::generate_packet_sequence(TMP_FILE_PATH, &p_, N_PACKETS,
                                  [](uint64_t i, vrt_packet* p) { p->fields.class_id.information_class_code = i % 4; });

    process();
}

TEST_F(StreamTest, PacketClassCode) {
    p_.header.has.class_id = true;
    vrt::generate_packet_sequence(TMP_FILE_PATH, &p_, N_PACKETS,
                                  [](uint64_t i, vrt_packet* p) { p->fields.class_id.packet_class_code = i % 4; });

    process();
}

TEST_F(StreamTest, StreamId) {
    p_.header.packet_type = VRT_PT_IF_DATA_WITH_STREAM_ID;
    vrt::generate_packet_sequence(TMP_FILE_PATH, &p_, N_PACKETS,
                                  [](uint64_t i, vrt_packet* p) { p->fields.stream_id = i % 4; });

    process();
}

TEST_F(StreamTest, All) {
    p_.header.packet_type  = VRT_PT_IF_DATA_WITH_STREAM_ID;
    p_.header.has.class_id = true;
    vrt::generate_packet_sequence(TMP_FILE_PATH, &p_, N_PACKETS, [](uint64_t i, vrt_packet* p) {
        p->fields.stream_id                       = i % 4;
        p->fields.class_id.oui                    = p->fields.stream_id;
        p->fields.class_id.information_class_code = static_cast<uint16_t>(p->fields.stream_id);
        p->fields.class_id.packet_class_code      = static_cast<uint16_t>(p->fields.stream_id);
    });

    process();
}

TEST_F(StreamTest, SomeClassIdDefault) {
    vrt::generate_packet_sequence(TMP_FILE_PATH, &p_, N_PACKETS, [](uint64_t i, vrt_packet* p) {
        p->header.has.class_id                    = i % 4 != 0;
        p->fields.class_id.oui                    = i % 2;
        p->fields.class_id.information_class_code = i % 2;
        p->fields.class_id.packet_class_code      = i % 2;
    });

    process();
}

TEST_F(StreamTest, SomeStreamIdDefault) {
    vrt::generate_packet_sequence(TMP_FILE_PATH, &p_, N_PACKETS, [](uint64_t i, vrt_packet* p) {
        if (i % 4 == 0) {
            p->header.packet_type = VRT_PT_IF_DATA_WITHOUT_STREAM_ID;
        } else {
            p->header.packet_type = VRT_PT_IF_DATA_WITH_STREAM_ID;
            p->fields.stream_id   = i % 4;
        }
    });

    process();
}

TEST_F(StreamTest, SomeClassStreamIdDefault) {
    vrt::generate_packet_sequence(TMP_FILE_PATH, &p_, N_PACKETS, [](uint64_t i, vrt_packet* p) {
        if (i % 3 == 0) {
            p->header.packet_type = VRT_PT_IF_DATA_WITHOUT_STREAM_ID;
        } else {
            p->header.packet_type = VRT_PT_IF_DATA_WITH_STREAM_ID;
            p->fields.stream_id   = i % 4;
        }
        if (i % 5 == 0) {
            p->header.has.class_id = false;
        } else {
            p->header.has.class_id                    = true;
            p->fields.class_id.oui                    = i % 2;
            p->fields.class_id.information_class_code = i % 2;
            p->fields.class_id.packet_class_code      = i % 2;
        }
    });

    process();
}
