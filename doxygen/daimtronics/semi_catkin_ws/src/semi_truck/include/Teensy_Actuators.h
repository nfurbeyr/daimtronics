// Generated by gencpp from file semi_truck/Teensy_Actuators.msg
// DO NOT EDIT!


#ifndef SEMI_TRUCK_MESSAGE_TEENSY_ACTUATORS_H
#define SEMI_TRUCK_MESSAGE_TEENSY_ACTUATORS_H


#include <string>
#include <vector>
#include <map>

#include <ros/types.h>
#include <ros/serialization.h>
#include <ros/builtin_message_traits.h>
#include <ros/message_operations.h>


namespace semi_truck
{
template <class ContainerAllocator>
struct Teensy_Actuators_
{
  typedef Teensy_Actuators_<ContainerAllocator> Type;

  Teensy_Actuators_()
    : motor_output(0)
    , steer_output(0)
    , fifth_output(0)  {
    }
  Teensy_Actuators_(const ContainerAllocator& _alloc)
    : motor_output(0)
    , steer_output(0)
    , fifth_output(0)  {
  (void)_alloc;
    }



   typedef int16_t _motor_output_type;
  _motor_output_type motor_output;

   typedef int16_t _steer_output_type;
  _steer_output_type steer_output;

   typedef int16_t _fifth_output_type;
  _fifth_output_type fifth_output;





  typedef boost::shared_ptr< ::semi_truck::Teensy_Actuators_<ContainerAllocator> > Ptr;
  typedef boost::shared_ptr< ::semi_truck::Teensy_Actuators_<ContainerAllocator> const> ConstPtr;

}; // struct Teensy_Actuators_

typedef ::semi_truck::Teensy_Actuators_<std::allocator<void> > Teensy_Actuators;

typedef boost::shared_ptr< ::semi_truck::Teensy_Actuators > Teensy_ActuatorsPtr;
typedef boost::shared_ptr< ::semi_truck::Teensy_Actuators const> Teensy_ActuatorsConstPtr;

// constants requiring out of line definition



template<typename ContainerAllocator>
std::ostream& operator<<(std::ostream& s, const ::semi_truck::Teensy_Actuators_<ContainerAllocator> & v)
{
ros::message_operations::Printer< ::semi_truck::Teensy_Actuators_<ContainerAllocator> >::stream(s, "", v);
return s;
}

} // namespace semi_truck

namespace ros
{
namespace message_traits
{



// BOOLTRAITS {'IsFixedSize': True, 'IsMessage': True, 'HasHeader': False}
// {'semi_truck': ['/home/nate/git/daimtronics/semi_catkin_ws/src/semi_truck/msg'], 'std_msgs': ['/opt/ros/kinetic/share/std_msgs/cmake/../msg']}

// !!!!!!!!!!! ['__class__', '__delattr__', '__dict__', '__doc__', '__eq__', '__format__', '__getattribute__', '__hash__', '__init__', '__module__', '__ne__', '__new__', '__reduce__', '__reduce_ex__', '__repr__', '__setattr__', '__sizeof__', '__str__', '__subclasshook__', '__weakref__', '_parsed_fields', 'constants', 'fields', 'full_name', 'has_header', 'header_present', 'names', 'package', 'parsed_fields', 'short_name', 'text', 'types']




template <class ContainerAllocator>
struct IsFixedSize< ::semi_truck::Teensy_Actuators_<ContainerAllocator> >
  : TrueType
  { };

template <class ContainerAllocator>
struct IsFixedSize< ::semi_truck::Teensy_Actuators_<ContainerAllocator> const>
  : TrueType
  { };

template <class ContainerAllocator>
struct IsMessage< ::semi_truck::Teensy_Actuators_<ContainerAllocator> >
  : TrueType
  { };

template <class ContainerAllocator>
struct IsMessage< ::semi_truck::Teensy_Actuators_<ContainerAllocator> const>
  : TrueType
  { };

template <class ContainerAllocator>
struct HasHeader< ::semi_truck::Teensy_Actuators_<ContainerAllocator> >
  : FalseType
  { };

template <class ContainerAllocator>
struct HasHeader< ::semi_truck::Teensy_Actuators_<ContainerAllocator> const>
  : FalseType
  { };


template<class ContainerAllocator>
struct MD5Sum< ::semi_truck::Teensy_Actuators_<ContainerAllocator> >
{
  static const char* value()
  {
    return "0d131da7355e429d9d8b9cc6b2375149";
  }

  static const char* value(const ::semi_truck::Teensy_Actuators_<ContainerAllocator>&) { return value(); }
  static const uint64_t static_value1 = 0x0d131da7355e429dULL;
  static const uint64_t static_value2 = 0x9d8b9cc6b2375149ULL;
};

template<class ContainerAllocator>
struct DataType< ::semi_truck::Teensy_Actuators_<ContainerAllocator> >
{
  static const char* value()
  {
    return "semi_truck/Teensy_Actuators";
  }

  static const char* value(const ::semi_truck::Teensy_Actuators_<ContainerAllocator>&) { return value(); }
};

template<class ContainerAllocator>
struct Definition< ::semi_truck::Teensy_Actuators_<ContainerAllocator> >
{
  static const char* value()
  {
    return "int16 motor_output\n\
int16 steer_output\n\
int16 fifth_output\n\
";
  }

  static const char* value(const ::semi_truck::Teensy_Actuators_<ContainerAllocator>&) { return value(); }
};

} // namespace message_traits
} // namespace ros

namespace ros
{
namespace serialization
{

  template<class ContainerAllocator> struct Serializer< ::semi_truck::Teensy_Actuators_<ContainerAllocator> >
  {
    template<typename Stream, typename T> inline static void allInOne(Stream& stream, T m)
    {
      stream.next(m.motor_output);
      stream.next(m.steer_output);
      stream.next(m.fifth_output);
    }

    ROS_DECLARE_ALLINONE_SERIALIZER
  }; // struct Teensy_Actuators_

} // namespace serialization
} // namespace ros

namespace ros
{
namespace message_operations
{

template<class ContainerAllocator>
struct Printer< ::semi_truck::Teensy_Actuators_<ContainerAllocator> >
{
  template<typename Stream> static void stream(Stream& s, const std::string& indent, const ::semi_truck::Teensy_Actuators_<ContainerAllocator>& v)
  {
    s << indent << "motor_output: ";
    Printer<int16_t>::stream(s, indent + "  ", v.motor_output);
    s << indent << "steer_output: ";
    Printer<int16_t>::stream(s, indent + "  ", v.steer_output);
    s << indent << "fifth_output: ";
    Printer<int16_t>::stream(s, indent + "  ", v.fifth_output);
  }
};

} // namespace message_operations
} // namespace ros

#endif // SEMI_TRUCK_MESSAGE_TEENSY_ACTUATORS_H
