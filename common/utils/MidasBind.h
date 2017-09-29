#ifndef MIDAS_MIDASBIND_H
#define MIDAS_MIDASBIND_H

#include <boost/bind.hpp>
//#include <functional>

// static constexpr auto& std_1 = std::placeholders::_1;
// static constexpr auto& std_2 = std::placeholders::_2;
// static constexpr auto& std_3 = std::placeholders::_3;
// static constexpr auto& std_4 = std::placeholders::_4;

#define MIDAS_BIND0(callback, objPtr) boost::bind(callback, objPtr)
#define MIDAS_BIND1(callback, objPtr) boost::bind(callback, objPtr, _1)
#define MIDAS_BIND2(callback, objPtr) boost::bind(callback, objPtr, _1, _2)
#define MIDAS_BIND3(callback, objPtr) boost::bind(callback, objPtr, _1, _2, _3)
#define MIDAS_BIND4(callback, objPtr) boost::bind(callback, objPtr, _1, _2, _3, _4)

#define MIDAS_STATIC_BIND0(callback) boost::bind(callback)
#define MIDAS_STATIC_BIND1(callback) boost::bind(callback, _1)
#define MIDAS_STATIC_BIND2(callback) boost::bind(callback, _1, _2)
#define MIDAS_STATIC_BIND3(callback) boost::bind(callback, _1, _2, _3)
#define MIDAS_STATIC_BIND4(callback) boost::bind(callback, _1, _2, _3, _4)

//#define MIDAS_STD_BIND0(callback, objPtr) std::bind(callback, objPtr)
//#define MIDAS_STD_BIND1(callback, objPtr) std::bind(callback, objPtr, std_1)
//#define MIDAS_STD_BIND2(callback, objPtr) std::bind(callback, objPtr, std_1, std_2)
//#define MIDAS_STD_BIND3(callback, objPtr) std::bind(callback, objPtr, std_1, std_2, std_3)
//#define MIDAS_STD_BIND4(callback, objPtr) std::bind(callback, objPtr, std_1, std_2, std_3, std_4)

#endif  // MIDAS_MIDASBIND_H
