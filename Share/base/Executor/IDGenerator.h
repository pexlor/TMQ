//
// IDGenerator.h
// IDGenerator
//
// 创建于 2022/3/28.
// 版权所有 (c) 腾讯. 保留所有权利。
//

#ifndef ID_GENERATOR_H
#define ID_GENERATOR_H

#include "Defines.h"

/// 常量值定义
// ID逐个递增
#define ID_INC 1
// int类型ID的最大值
#define ID_INT_MAX 0xFFFFFFFF
// int类型ID的最小值，从1开始
#define ID_INT_MIN 0x00000001

/**
 * ID生成器，该类提供使用原子操作的通用ID。
 */
class IDGenerator {
private:
 // 主题递增的ID
 TMQId tid;
 // tmq消息递增的ID
 TMQMsgId mid;
public:
 /**
 * 获取一个主题ID，这将使tid增加1。
 * @return 一个无符号整数值表示主题ID。
 */
 TMQId GetTopicId();

 /**
 * 设置主题ID
 * @param id，要设置的ID。
 */
 void SetTopicId(TMQId id);

 /**
 * 获取一个tmq消息的ID，这将使mid增加1。
 * @return 一个无符号的tmq消息ID。
 */
 TMQMsgId GetMsgId();

 /**
 * 设置消息ID。
 * @param id，要设置的ID。
 */
 void SetMsgId(TMQMsgId id);

 /**
 * IDGenerator实例的静态单例方法。
 * @return 一个指向IDGenerator实例的指针。
 */
 static IDGenerator *GetInstance();
};


#endif //ID_GENERATOR_H