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
    typedef struct _ui_tree_node
    {
        /* 节点标识 */
        char *name;
        int id;

        /* 位置大小信息 */
        rect_t rect;
        int32_t text_size;
        /* 树形结构指针 */
        struct _ui_tree_node *parent; // 父节点

        /* 子节点数组（替换原来的链表结构） */
        struct _ui_tree_node **children; // 子节点指针数组
        int child_count;                 // 当前子节点数量
        int child_capacity;              // 子节点数组容量

        /* 节点数据 */
        void *user_data;

    } ui_tree_node;

    /* 函数声明 */

    /**
     * @brief 创建新的UI树节点
     * @param name 节点名称
     * @param rect 节点矩形区域
     * @param id 节点ID
     * @return 新创建的节点指针
     */
    ui_tree_node *ui_tree_node_create(const char *name, rect_t rect, int id);

    /**
     * @brief 释放节点及其所有子节点
     * @param node 要释放的节点
     */
    void ui_tree_node_free(ui_tree_node *node);

    /**
     * @brief 添加子节点
     * @param parent 父节点
     * @param child 要添加的子节点
     */
    void ui_tree_node_add_child(ui_tree_node *parent, ui_tree_node *child);

    /**
     * @brief 从父节点移除子节点（不释放内存）
     * @param parent 父节点
     * @param child 要移除的子节点
     */
    void ui_tree_node_remove_child(ui_tree_node *parent, ui_tree_node *child);

    /**
     * @brief 在指定位置插入子节点
     * @param parent 父节点
     * @param child 要插入的子节点
     * @param index 插入位置索引
     * @return 成功返回0，失败返回-1
     */
    int ui_tree_node_insert_child(ui_tree_node *parent, ui_tree_node *child, int index);

    /**
     * @brief 获取子节点数量
     * @param parent 父节点
     * @return 子节点数量
     */
    int ui_tree_node_get_child_count(const ui_tree_node *parent);

    /**
     * @brief 根据索引获取子节点
     * @param parent 父节点
     * @param index 子节点索引
     * @return 子节点指针，失败返回NULL
     */
    ui_tree_node *ui_tree_node_get_child_at(const ui_tree_node *parent, int index);

    /**
     * @brief 根据名称查找节点（深度优先搜索）
     * @param root 根节点
     * @param name 要查找的节点名称
     * @return 找到的节点指针，未找到返回NULL
     */
    ui_tree_node *ui_tree_node_find_by_name(const ui_tree_node *root, const char *name);

    /**
     * @brief 根据ID查找节点（深度优先搜索）
     * @param root 根节点
     * @param id 要查找的节点ID
     * @return 找到的节点指针，未找到返回NULL
     */
    ui_tree_node *ui_tree_node_find_by_id(const ui_tree_node *root, int id);

    /**
     * @brief 遍历树（深度优先）
     * @param root 根节点
     * @param callback 回调函数，接收节点和深度参数
     * @param user_data 用户数据
     */
    void ui_tree_node_traverse_dfs(const ui_tree_node *root,
                                   void (*callback)(const ui_tree_node *, int, void *),
                                   void *user_data);

    /**
     * @brief 遍历树（广度优先）
     * @param root 根节点
     * @param callback 回调函数，接收节点和深度参数
     * @param user_data 用户数据
     */
    void ui_tree_node_traverse_bfs(const ui_tree_node *root,
                                   void (*callback)(const ui_tree_node *, int, void *),
                                   void *user_data);

    /**
     * @brief 获取节点深度
     * @param node 节点
     * @return 节点深度（根节点为0）
     */
    int ui_tree_node_get_depth(const ui_tree_node *node);

    /**
     * @brief 获取树的高度
     * @param root 根节点
     * @return 树的高度
     */
    int ui_tree_node_get_height(const ui_tree_node *root);

    /**
     * @brief 检查节点是否是另一个节点的后代
     * @param node 要检查的节点
     * @param ancestor 可能的祖先节点
     * @return 如果是后代返回1，否则返回0
     */
    int ui_tree_node_is_descendant(const ui_tree_node *node, const ui_tree_node *ancestor);

    /**
     * @brief 设置节点用户数据
     * @param node 节点
     * @param user_data 用户数据
     */
    void ui_tree_node_set_user_data(ui_tree_node *node, void *user_data);

    /**
     * @brief 获取节点用户数据
     * @param node 节点
     * @return 用户数据指针
     */
    void *ui_tree_node_get_user_data(const ui_tree_node *node);

    /**
     * @brief 克隆节点及其子树
     * @param node 要克隆的节点
     * @return 克隆后的新节点
     */
    ui_tree_node *ui_tree_node_clone(const ui_tree_node *node);

    /**
     * @brief 从widget树初始化UI树节点结构
     * @param parent 父widget
     * @param parent_node 要初始化的UI树节点
     * @param level 当前层级
     */
    void init_child_recursive(widget_t *parent, ui_tree_node *parent_node, int32_t level);

    /**
     * @brief 从根widget创建完整的UI树
     * @param root_widget 根widget
     * @return 创建的UI树根节点
     */
    ui_tree_node *ui_tree_node_create_from_widget_tree(widget_t *root_widget);

    /**
     * @brief 打印树结构
     * @param root 根节点
     */
    void ui_tree_node_print_tree(const ui_tree_node *root);

#ifdef __cplusplus
}
#endif

#endif /* UI_TREE_NODE_H */