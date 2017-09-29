#include "catch.hpp"
#include "net/buffer/ConstBuffer.h"
#include "net/buffer/ConstBufferList.h"
#include "net/buffer/ConstBufferSequence.h"
#include "net/buffer/MfBuffer.h"
#include "net/buffer/MutableBufferStream.h"

using namespace midas;

TEST_CASE("ConstBuffer heap", "[ConstBuffer]") {
    static const char string1[] = "hello world.";

    {
        ConstBuffer buffer(string1, sizeof(string1));
        REQUIRE(buffer.size() == sizeof(string1));
        REQUIRE(memcmp(buffer.data(), string1, sizeof(string1)) == 0);
    }

    {
        MutableBuffer buffer(sizeof(string1));
        buffer.store(string1, sizeof(string1));

        ConstBuffer cBuffer(buffer);
        REQUIRE(cBuffer.size() == sizeof(string1));
        REQUIRE(memcmp(cBuffer.data(), string1, sizeof(string1)) == 0);

        ConstBuffer cBuffer1(cBuffer);
        REQUIRE(cBuffer1.size() == sizeof(string1));
        REQUIRE(memcmp(cBuffer1.data(), string1, sizeof(string1)) == 0);
        REQUIRE(cBuffer1.data() == cBuffer.data());

        ConstBuffer cBuffer2;
        REQUIRE(cBuffer2.size() == 0);
        cBuffer2.swap(cBuffer1);
        REQUIRE(cBuffer1.size() == 0);
        REQUIRE(memcmp(cBuffer2.data(), string1, sizeof(string1)) == 0);
    }
}

TEST_CASE("MutableBuffer heap", "[MutableBuffer]") {
    static const char string1[] = "hello world.";

    MutableBuffer buffer(sizeof(string1));
    REQUIRE(buffer.capacity() == sizeof(string1));

    buffer.store(string1, sizeof(string1));
    REQUIRE(memcmp(buffer.data(), string1, sizeof(string1)) == 0);

    // extend at backend
    static const char string2[] = "hello world buffer.";
    buffer.store(string2, sizeof(string2));
    const char* p = buffer.data();
    REQUIRE(memcmp(p, string1, sizeof(string1)) == 0);
    REQUIRE(memcmp(p + sizeof(string1), string2, sizeof(string2)) == 0);
    REQUIRE(buffer.size() == sizeof(string1) + sizeof(string2));
    REQUIRE(buffer.capacity() >= sizeof(string1) + sizeof(string2));

    // write
    buffer.clear();
    mempcpy(buffer.write_buffer<char*>(), string1, sizeof(string1));
    buffer.store(sizeof(string1));
    REQUIRE(memcmp(buffer.data(), string1, sizeof(string1)) == 0);

    mempcpy(buffer.write_buffer<char*>(), string2, sizeof(string2));
    buffer.store(sizeof(string2));

    p = buffer.data();
    REQUIRE(memcmp(p, string1, sizeof(string1)) == 0);
    REQUIRE(memcmp(p + sizeof(string1), string2, sizeof(string2)) == 0);
    REQUIRE(buffer.size() == sizeof(string1) + sizeof(string2));
    REQUIRE(buffer.capacity() >= sizeof(string1) + sizeof(string2));

    {
        // check buffer copy
        MutableBuffer copy = buffer;
        REQUIRE(buffer.data() == copy.data());
        REQUIRE(buffer.size() == copy.size());
    }

    {
        // check buffer copy with offset
        MutableBuffer copy(buffer, sizeof(string1));
        REQUIRE(memcmp(copy.data(), string2, sizeof(string2)) == 0);
        REQUIRE(copy.size() == sizeof(string2));
    }

    {
        // check buffer copy with offset and destination offset
        MutableBuffer copy(buffer, sizeof(string1), sizeof(string1));
        mempcpy(buffer.write_buffer<char*>(), string1, sizeof(string1));
        p = buffer.data();

        REQUIRE(memcmp(p, string1, sizeof(string1)) == 0);
        REQUIRE(memcmp(p + sizeof(string1), string2, sizeof(string2)) == 0);
        REQUIRE(buffer.size() == sizeof(string1) + sizeof(string2));
    }
}

TEST_CASE("MutableBuffer pool", "[MutableBuffer]") {
    static const char string1[] = "hello world.";
    static const size_t BufferSize = sizeof(string1);

    MutableBuffer buffer(BufferSize, PoolAllocator<BufferSize>());

    REQUIRE(buffer.capacity() == BufferSize);

    buffer.store(string1, BufferSize);
    REQUIRE(memcmp(buffer.data(), string1, BufferSize) == 0);

    // extend at backend
    static const char string2[] = "hello world buffer.";
    buffer.store(string2, sizeof(string2));
    const char* p = buffer.data();
    REQUIRE(memcmp(p, string1, BufferSize) == 0);
    REQUIRE(memcmp(p + BufferSize, string2, sizeof(string2)) == 0);
    REQUIRE(buffer.size() == BufferSize + sizeof(string2));
    REQUIRE(buffer.capacity() >= BufferSize + sizeof(string2));

    // write
    buffer.clear();

    // reallocate
    const size_t BufferSize1 = 128;
    for (int i = 0; i < 1024; ++i) {
        MutableBuffer buffer1(BufferSize1, PoolAllocator<BufferSize1>());
        buffer1.store(string1, BufferSize);
        REQUIRE(memcmp(buffer1.data(), string1, BufferSize) == 0);
        REQUIRE(buffer1.size() == BufferSize);
    }
}

TEST_CASE("MutableBuffer fast pool", "[MutableBuffer]") {
    static const char string1[] = "hello world.";
    static const size_t BufferSize = sizeof(string1);

    MutableBuffer buffer(BufferSize, FastPoolAllocator<BufferSize>());

    REQUIRE(buffer.capacity() == BufferSize);

    buffer.store(string1, BufferSize);
    REQUIRE(memcmp(buffer.data(), string1, BufferSize) == 0);

    // extend at backend
    static const char string2[] = "hello world buffer.";
    buffer.store(string2, sizeof(string2));
    const char* p = buffer.data();
    REQUIRE(memcmp(p, string1, BufferSize) == 0);
    REQUIRE(memcmp(p + BufferSize, string2, sizeof(string2)) == 0);
    REQUIRE(buffer.size() == BufferSize + sizeof(string2));
    REQUIRE(buffer.capacity() >= BufferSize + sizeof(string2));

    // write
    buffer.clear();

    // reallocate
    const size_t BufferSize1 = 128;
    for (int i = 0; i < 1024; ++i) {
        MutableBuffer buffer1(BufferSize1, FastPoolAllocator<BufferSize1>());
        buffer1.store(string1, BufferSize);
        REQUIRE(memcmp(buffer1.data(), string1, BufferSize) == 0);
        REQUIRE(buffer1.size() == BufferSize);
    }
}

TEST_CASE("ConstBufferSequence", "[ConstBufferSequence]") {
    ConstBufferSequence cbs;
    REQUIRE(cbs.size() == 0);
    for (size_t i = 0; i < 1024; ++i) {
        MutableBuffer buffer(128);
        buffer.store((size_t)snprintf(buffer.write_buffer<char*>(), 128, "test in number %zu", i));
        cbs.push_back(buffer);
    }

    REQUIRE(cbs.size() != 0);
    size_t count = 0, totalSize = 0;
    char tmp[128];
    for (auto itr = cbs.begin(); itr != cbs.end(); ++itr, ++count) {
        size_t n = (size_t)snprintf(tmp, sizeof(tmp), "test in number %zu", count);
        REQUIRE(itr->size() == n);
        REQUIRE(memcmp(itr->data(), tmp, itr->size()) == 0);
        totalSize += n;
    }
    REQUIRE(cbs.byteSize == totalSize);

    cbs.clear();
    REQUIRE(cbs.size() == 0);
}

TEST_CASE("ConstBufferList", "[ConstBufferList]") {
    ConstBufferList list;
    REQUIRE(list.empty());
    for (size_t i = 0; i < 1024; ++i) {
        MutableBuffer buffer(128);
        buffer.store((size_t)snprintf(buffer.write_buffer<char*>(), 128, "test in number %zu", i));
        list.push_back(buffer);
    }

    REQUIRE(list.size == 1024);
    size_t count = 0, totalSize = 0;
    char tmp[128];
    for (auto itr = list.begin(); itr != list.end(); ++itr, ++count) {
        size_t n = (size_t)snprintf(tmp, sizeof(tmp), "test in number %zu", count);
        REQUIRE((*itr).size() == n);
        REQUIRE(memcmp((*itr).data(), tmp, (*itr).size()) == 0);
        totalSize += n;
    }
    REQUIRE(list.byteSize == totalSize);

    list.clear();
    REQUIRE(list.size == 0);
}

TEST_CASE("MutableBufferStream", "[MutableBufferStream]") {
    static const char test1[] = "hello 1";
    static const char test2[] = "hello world 2";

    MutableBuffer buffer(5);
    MutableBufferOStream mbos(buffer);
    ostream os(&mbos);

    os << test1;
    os.flush();

    REQUIRE(memcmp(buffer.data(), test1, strlen(test1)) == 0);
    REQUIRE(buffer.size() == strlen(test1));
    REQUIRE(buffer.capacity() >= strlen(test1));

    os << test2;
    os.flush();
    const char* p = buffer.data();

    REQUIRE(memcmp(p, test1, strlen(test1)) == 0);
    REQUIRE(memcmp(p + strlen(test1), test2, strlen(test2)) == 0);
    REQUIRE(buffer.size() == strlen(test1) + strlen(test2));
    REQUIRE(buffer.capacity() >= strlen(test1) + strlen(test2));
}

TEST_CASE("MfBuffer", "[MfBuffer]") {
    static const char test1[] = "test1";

    ConstBuffer buffers[10];
    string mfStrings[10];

    MfBufferAlloc<> mfBuffer;
    for (int i = 0; i < 10; ++i) {
        mfBuffer.start(test1);

        mfStrings[i] = "id test1";

        char tmp[20];
        for (int j = 0; j < 100; ++j) {
            sprintf(tmp, "%c%d", 'f' + i, j);
            mfBuffer.add_int(tmp, j);
            sprintf(tmp, " ,%c%d %d", 'f' + i, j, j);
            mfStrings[i] += tmp;
        }
        mfStrings[i] += " ,;";
        mfBuffer.finish();
        buffers[i] = mfBuffer.buffer();

        REQUIRE(string(buffers[i].data() + MIDAS_HEADER_SIZE, buffers[i].size() - MIDAS_HEADER_SIZE) == mfStrings[i]);
    }
}

TEST_CASE("MfBuffer1", "[MfBuffer1]") {
    MfBuffer mf;
    REQUIRE(mf.mf_string() == "");
    mf.start("test_id");
    REQUIRE(mf.mf_string() == "id test_id ,");
    mf.finish();
    REQUIRE(mf.mf_string() == "id test_id ,;");

    // reuse
    mf.start("test_id1");
    REQUIRE(mf.mf_string() == "id test_id1 ,");
    mf.finish();
    REQUIRE(mf.mf_string() == "id test_id1 ,;");

    mf.start("test_id2");
    REQUIRE(mf.mf_string() == "id test_id2 ,");
    mf.add_int("int", 42);
    REQUIRE(mf.mf_string() == "id test_id2 ,int 42 ,");
    mf.add_float("float1", 42.123);
    REQUIRE(mf.mf_string() == "id test_id2 ,int 42 ,float1 42.123 ,");
    mf.add_float("float2", 42.123456789);
    REQUIRE(mf.mf_string() == "id test_id2 ,int 42 ,float1 42.123 ,float2 42.123457 ,");

    mf.start("test_id3");
    mf.add_float("float3", 42.123456789, 5);
    REQUIRE(mf.mf_string() == "id test_id3 ,float3 42.12346 ,");
    mf.add_float("float4", 42.123456789, 7);
    REQUIRE(mf.mf_string() == "id test_id3 ,float3 42.12346 ,float4 42.1234568 ,");
    mf.add_float("float5", 42.123, 7);
    REQUIRE(mf.mf_string() == "id test_id3 ,float3 42.12346 ,float4 42.1234568 ,float5 42.123 ,");

    mf.start("test_id4");
    mf.add_fixed_point("fp1", 123, 0);
    REQUIRE(mf.mf_string() == "id test_id4 ,fp1 123 ,");
    mf.start("test_id4");
    mf.add_fixed_point("fp1", 123, 1);
    REQUIRE(mf.mf_string() == "id test_id4 ,fp1 12.3 ,");
    mf.start("test_id4");
    mf.add_fixed_point("fp1", 123, 2);
    REQUIRE(mf.mf_string() == "id test_id4 ,fp1 1.23 ,");
    mf.start("test_id4");
    mf.add_fixed_point("fp1", 123, 3);
    REQUIRE(mf.mf_string() == "id test_id4 ,fp1 0.123 ,");
    mf.start("test_id4");
    mf.add_fixed_point("fp1", 123, 4);
    REQUIRE(mf.mf_string() == "id test_id4 ,fp1 0.0123 ,");
    mf.start("test_id4");
    mf.add_fixed_point("fp1", -123456, 4);
    REQUIRE(mf.mf_string() == "id test_id4 ,fp1 -12.3456 ,");

    mf.start("test_id5");
    mf.add_fixed_point_trim("fp1", 100, 0);
    REQUIRE(mf.mf_string() == "id test_id5 ,fp1 100 ,");
    mf.start("test_id5");
    mf.add_fixed_point_trim("fp1", 100, 1);
    REQUIRE(mf.mf_string() == "id test_id5 ,fp1 10 ,");
    mf.start("test_id5");
    mf.add_fixed_point_trim("fp1", 100, 2);
    REQUIRE(mf.mf_string() == "id test_id5 ,fp1 1 ,");
    mf.start("test_id5");
    mf.add_fixed_point_trim("fp1", 100, 3);
    REQUIRE(mf.mf_string() == "id test_id5 ,fp1 0.1 ,");
    mf.start("test_id5");
    mf.add_fixed_point_trim("fp1", 100, 4);
    REQUIRE(mf.mf_string() == "id test_id5 ,fp1 0.01 ,");
    mf.start("test_id5");
    mf.add_fixed_point_trim("fp1", -123456, 4);
    REQUIRE(mf.mf_string() == "id test_id5 ,fp1 -12.3456 ,");

    mf.start("test_id6");
    mf.add_timestamp("t", 123, 456);
    REQUIRE(mf.mf_string() == "id test_id6 ,t 123.000456 ,");
    mf.start("test_id6");
    mf.add_time_HM("t", 23, 59);
    REQUIRE(mf.mf_string() == "id test_id6 ,t 2359 ,");
    mf.start("test_id6");
    mf.add_time_HM("t", 1, 1);
    REQUIRE(mf.mf_string() == "id test_id6 ,t 0101 ,");
    mf.start("test_id6");
    mf.add_time_HMS("t", 23, 59, 59);
    REQUIRE(mf.mf_string() == "id test_id6 ,t 235959 ,");
    mf.start("test_id6");
    mf.add_time_HMS("t", 1, 1, 1);
    REQUIRE(mf.mf_string() == "id test_id6 ,t 010101 ,");

    mf.start("test_id7");
    mf.add_date("dt", 2017, 8, 13);
    REQUIRE(mf.mf_string() == "id test_id7 ,dt 20170813 ,");

    mf.start("test_id8");
    mf.add("empty");
    REQUIRE(mf.mf_string() == "id test_id8 ,empty ,");

    mf.start("test_id9");
    mf.add_price("price", 100, 1);
    REQUIRE(mf.mf_string() == "id test_id9 ,price 10 ,");
    mf.start("test_id9");
    mf.add_price("price", 100, 2);
    REQUIRE(mf.mf_string() == "id test_id9 ,price 1 ,");
    mf.start("test_id9");
    mf.add_price("price", 100, 3);
    REQUIRE(mf.mf_string() == "id test_id9 ,price 0.1 ,");

    mf.start("test_id10");
    mf.add_microsecond_intraday("intraday", 10000000);
    REQUIRE(mf.mf_string() == "id test_id10 ,intraday 024640.000 ,");

    mf.start("test_id11");
    mf.add_date("date", 20170817);
    REQUIRE(mf.mf_string() == "id test_id11 ,date 20170817 ,");
}

TEST_CASE("MfBufferSize", "[MfBufferSize]") {
    MfBuffer mf;
    REQUIRE(mf.size() == 0);
    REQUIRE(mf.capacity() == 0);

    mf.start("a");
    REQUIRE(mf.capacity() == 64);

    for (int i = 0; i < 1000; ++i) {
        mf.add("abcde" + int2str(i));
    }
    mf.finish();
    REQUIRE(mf.size() == 9911);
    REQUIRE(mf.capacity() == 16384);

    MfBuffer mf2;
    mf2.reserve(10000);
    REQUIRE(mf2.size() == 0);
    REQUIRE(mf2.capacity() == 10000);

    mf2.start("a");
    REQUIRE(mf2.capacity() == 10000);

    for (int i = 0; i < 1000; ++i) {
        mf2.add("abcde" + int2str(i));
    }
    mf2.finish();
    REQUIRE(mf2.size() == 9911);
    REQUIRE(mf2.capacity() == 10000);

    REQUIRE(mf.mf_string() == mf2.mf_string());
}
