#ifndef MUDUO_BASE_COPYABLE_H
#define MUDUO_BASE_COPYABLE_H

namespace muduo
{

/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of copyable should be a value type.
class copyable
{
    // 空基类，标识类，值类型
    // C++中两种语义：
    // 值语义：可拷贝，拷贝后与原对象脱离关系
    // 对象语义：要么不可以拷贝，要么可以拷贝，但与原对象仍有关系（如共享底层资源）
};

};

#endif  // MUDUO_BASE_COPYABLE_H
