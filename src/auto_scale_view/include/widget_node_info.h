#ifndef UI_TREE_NODE_H
#define UI_TREE_NODE_H

#include <stdlib.h>
#include <string.h>
#include "base/widget.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* UI树节点结构体 - 使用数组存储子节点 */
    typedef struct _widget_node_info
    {
        /* 节点标识 */
        char *name;
        int id;

        /* 位置大小信息 */
        rect_t rect;
        int32_t text_size;
        /* 树形结构指针 */
        struct _widget_node_info *parent; // 父节点

        /* 子节点数组（替换原来的链表结构） */
        struct _widget_node_info **children; // 子节点指针数组
        int child_count;                 // 当前子节点数量
        int child_capacity;              // 子节点数组容量

        /* 节点数据 */
        void *user_data;

    } widget_node_info;

    /* 函数声明 */

    /**
     * @brief 创建新的UI树节点
     * @param name 节点名称
     * @param rect 节点矩形区域
     * @param id 节点ID
     * @return 新创建的节点指针
     */
    widget_node_info *widget_node_info_create(const char *name, rect_t rect, int id);

    /**
     * @brief 释放节点及其所有子节点
     * @param node 要释放的节点
     */
    void widget_node_info_free(widget_node_info *node);

    /**
     * @brief 添加子节点
     * @param parent 父节点
     * @param child 要添加的子节点
     */
    void widget_node_info_add_child(widget_node_info *parent, widget_node_info *child);

    /**
     * @brief 从父节点移除子节点（不释放内存）
     * @param parent 父节点
     * @param child 要移除的子节点
     */
    void widget_node_info_remove_child(widget_node_info *parent, widget_node_info *child);

    /**
     * @brief 在指定位置插入子节点
     * @param parent 父节点
     * @param child 要插入的子节点
     * @param index 插入位置索引
     * @return 成功返回0，失败返回-1
     */
    int widget_node_info_insert_child(widget_node_info *parent, widget_node_info *child, int index);

    /**
     * @brief 获取子节点数量
     * @param parent 父节点
     * @return 子节点数量
     */
    int widget_node_info_get_child_count(const widget_node_info *parent);

    /**
     * @brief 根据索引获取子节点
     * @param parent 父节点
     * @param index 子节点索引
     * @return 子节点指针，失败返回NULL
     */
    widget_node_info *widget_node_info_get_child_at(const widget_node_info *parent, int index);

    /**
     * @brief 根据名称查找节点（深度优先搜索）
     * @param root 根节点
     * @param name 要查找的节点名称
     * @return 找到的节点指针，未找到返回NULL
     */
    widget_node_info *widget_node_info_find_by_name(const widget_node_info *root, const char *name);

    /**
     * @brief 根据ID查找节点（深度优先搜索）
     * @param root 根节点
     * @param id 要查找的节点ID
     * @return 找到的节点指针，未找到返回NULL
     */
    widget_node_info *widget_node_info_find_by_id(const widget_node_info *root, int id);

    /**
     * @brief 遍历树（深度优先）
     * @param root 根节点
     * @param callback 回调函数，接收节点和深度参数
     * @param user_data 用户数据
     */
    void widget_node_info_traverse_dfs(const widget_node_info *root,
                                   void (*callback)(const widget_node_info *, int, void *),
                                   void *user_data);

    /**
     * @brief 遍历树（广度优先）
     * @param root 根节点
     * @param callback 回调函数，接收节点和深度参数
     * @param user_data 用户数据
     */
    void widget_node_info_traverse_bfs(const widget_node_info *root,
                                   void (*callback)(const widget_node_info *, int, void *),
                                   void *user_data);

    /**
     * @brief 获取节点深度
     * @param node 节点
     * @return 节点深度（根节点为0）
     */
    int widget_node_info_get_depth(const widget_node_info *node);

    /**
     * @brief 获取树的高度
     * @param root 根节点
     * @return 树的高度
     */
    int widget_node_info_get_height(const widget_node_info *root);

    /**
     * @brief 检查节点是否是另一个节点的后代
     * @param node 要检查的节点
     * @param ancestor 可能的祖先节点
     * @return 如果是后代返回1，否则返回0
     */
    int widget_node_info_is_descendant(const widget_node_info *node, const widget_node_info *ancestor);

    /**
     * @brief 设置节点用户数据
     * @param node 节点
     * @param user_data 用户数据
     */
    void widget_node_info_set_user_data(widget_node_info *node, void *user_data);

    /**
     * @brief 获取节点用户数据
     * @param node 节点
     * @return 用户数据指针
     */
    void *widget_node_info_get_user_data(const widget_node_info *node);

    /**
     * @brief 克隆节点及其子树
     * @param node 要克隆的节点
     * @return 克隆后的新节点
     */
    widget_node_info *widget_node_info_clone(const widget_node_info *node);

    /**
     * @brief 从widget树初始化UI树节点结构
     * @param parent 父widget
     * @param parent_node 要初始化的UI树节点
     * @param level 当前层级
     */
    void init_child_recursive(widget_t *parent, widget_node_info *parent_node, int32_t level);

    /**
     * @brief 从根widget创建完整的UI树
     * @param root_widget 根widget
     * @return 创建的UI树根节点
     */
    widget_node_info *widget_node_info_create_from_widget_tree(widget_t *root_widget);

    /**
     * @brief 打印树结构
     * @param root 根节点
     */
    void widget_node_info_print_tree(const widget_node_info *root);

#ifdef __cplusplus
}
#endif

#endif /* UI_TREE_NODE_H */