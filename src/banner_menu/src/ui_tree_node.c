#include "ui_tree_node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 内部队列结构，用于广度优先遍历 */
typedef struct _node_queue
{
    const ui_tree_node **nodes;
    int front;
    int rear;
    int capacity;
} node_queue;

static node_queue *queue_create(int capacity)
{
    node_queue *q = malloc(sizeof(node_queue));
    q->nodes = malloc(sizeof(ui_tree_node *) * capacity);
    q->front = 0;
    q->rear = 0;
    q->capacity = capacity;
    return q;
}

static void queue_free(node_queue *q)
{
    free(q->nodes);
    free(q);
}

static void queue_push(node_queue *q, const ui_tree_node *node)
{
    if (q->rear < q->capacity)
    {
        q->nodes[q->rear++] = node;
    }
}

static const ui_tree_node *queue_pop(node_queue *q)
{
    if (q->front < q->rear)
    {
        return q->nodes[q->front++];
    }
    return NULL;
}

static int queue_is_empty(node_queue *q)
{
    return q->front >= q->rear;
}

/* 内部辅助函数 */
static int ensure_child_capacity(ui_tree_node *parent, int required_capacity)
{
    if (!parent)
        return 0;

    if (parent->child_capacity >= required_capacity)
    {
        return 1;
    }

    int new_capacity = parent->child_capacity == 0 ? 8 : parent->child_capacity * 2;
    if (new_capacity < required_capacity)
    {
        new_capacity = required_capacity;
    }

    ui_tree_node **new_children = realloc(parent->children, sizeof(ui_tree_node *) * new_capacity);
    if (!new_children)
        return 0;

    parent->children = new_children;
    parent->child_capacity = new_capacity;
    return 1;
}

/* 实现函数 */

ui_tree_node *ui_tree_node_create(const char *name, rect_t rect, int id)
{
    ui_tree_node *node = (ui_tree_node *)malloc(sizeof(ui_tree_node));
    if (!node)
        return NULL;

    node->name = name ? strdup(name) : NULL;
    node->id = id;
    node->rect = rect;
    node->parent = NULL;
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    node->user_data = NULL;

    return node;
}

void ui_tree_node_free(ui_tree_node *node)
{
    if (!node)
        return;

    /* 递归释放所有子节点 */
    for (int i = 0; i < node->child_count; i++)
    {
        ui_tree_node_free(node->children[i]);
    }

    /* 释放子节点数组 */
    free(node->children);

    /* 释放节点自身资源 */
    free(node->name);
    free(node);
}

void ui_tree_node_add_child(ui_tree_node *parent, ui_tree_node *child)
{
    if (!parent || !child)
        return;

    /* 确保容量足够 */
    if (!ensure_child_capacity(parent, parent->child_count + 1))
    {
        return;
    }

    /* 移除child从原来的父节点 */
    if (child->parent)
    {
        ui_tree_node_remove_child(child->parent, child);
    }

    /* 添加到子节点数组末尾 */
    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

void ui_tree_node_remove_child(ui_tree_node *parent, ui_tree_node *child)
{
    if (!parent || !child || child->parent != parent)
        return;

    /* 在数组中查找子节点 */
    int found_index = -1;
    for (int i = 0; i < parent->child_count; i++)
    {
        if (parent->children[i] == child)
        {
            found_index = i;
            break;
        }
    }

    if (found_index == -1)
        return;

    /* 将后面的元素前移 */
    for (int i = found_index; i < parent->child_count - 1; i++)
    {
        parent->children[i] = parent->children[i + 1];
    }

    parent->child_count--;
    child->parent = NULL;
}

int ui_tree_node_insert_child(ui_tree_node *parent, ui_tree_node *child, int index)
{
    if (!parent || !child || index < 0 || index > parent->child_count)
        return -1;

    /* 确保容量足够 */
    if (!ensure_child_capacity(parent, parent->child_count + 1))
    {
        return -1;
    }

    /* 移除child从原来的父节点 */
    if (child->parent)
    {
        ui_tree_node_remove_child(child->parent, child);
    }

    /* 移动元素腾出空间 */
    for (int i = parent->child_count; i > index; i--)
    {
        parent->children[i] = parent->children[i - 1];
    }

    /* 插入新节点 */
    parent->children[index] = child;
    parent->child_count++;
    child->parent = parent;

    return 0;
}

int ui_tree_node_get_child_count(const ui_tree_node *parent)
{
    return parent ? parent->child_count : 0;
}

ui_tree_node *ui_tree_node_get_child_at(const ui_tree_node *parent, int index)
{
    if (!parent || index < 0 || index >= parent->child_count)
        return NULL;
    return parent->children[index];
}

ui_tree_node *ui_tree_node_find_by_name(const ui_tree_node *root, const char *name)
{
    if (!root || !name)
        return NULL;

    /* 检查当前节点 */
    if (root->name && strcmp(root->name, name) == 0)
    {
        return (ui_tree_node *)root;
    }

    /* 递归检查所有子节点 */
    for (int i = 0; i < root->child_count; i++)
    {
        ui_tree_node *found = ui_tree_node_find_by_name(root->children[i], name);
        if (found)
            return found;
    }

    return NULL;
}

ui_tree_node *ui_tree_node_find_by_id(const ui_tree_node *root, int id)
{
    if (!root)
        return NULL;

    /* 检查当前节点 */
    if (root->id == id)
    {
        return (ui_tree_node *)root;
    }

    /* 递归检查所有子节点 */
    for (int i = 0; i < root->child_count; i++)
    {
        ui_tree_node *found = ui_tree_node_find_by_id(root->children[i], id);
        if (found)
            return found;
    }

    return NULL;
}

static void traverse_dfs_recursive(const ui_tree_node *node, int depth,
                                   void (*callback)(const ui_tree_node *, int, void *),
                                   void *user_data)
{
    if (!node)
        return;

    callback(node, depth, user_data);

    /* 遍历所有子节点 */
    for (int i = 0; i < node->child_count; i++)
    {
        traverse_dfs_recursive(node->children[i], depth + 1, callback, user_data);
    }
}

void ui_tree_node_traverse_dfs(const ui_tree_node *root,
                               void (*callback)(const ui_tree_node *, int, void *),
                               void *user_data)
{
    if (!root || !callback)
        return;
    traverse_dfs_recursive(root, 0, callback, user_data);
}

void ui_tree_node_traverse_bfs(const ui_tree_node *root,
                               void (*callback)(const ui_tree_node *, int, void *),
                               void *user_data)
{
    if (!root || !callback)
        return;

    node_queue *q = queue_create(100); /* 初始容量100 */
    queue_push(q, root);

    int current_depth = 0;
    int nodes_at_current_level = 1;
    int nodes_at_next_level = 0;

    while (!queue_is_empty(q))
    {
        const ui_tree_node *node = queue_pop(q);
        nodes_at_current_level--;

        callback(node, current_depth, user_data);

        /* 将子节点加入队列 */
        for (int i = 0; i < node->child_count; i++)
        {
            queue_push(q, node->children[i]);
            nodes_at_next_level++;
        }

        if (nodes_at_current_level == 0)
        {
            current_depth++;
            nodes_at_current_level = nodes_at_next_level;
            nodes_at_next_level = 0;
        }
    }

    queue_free(q);
}

int ui_tree_node_get_depth(const ui_tree_node *node)
{
    if (!node)
        return -1;

    int depth = 0;
    ui_tree_node *parent = node->parent;
    while (parent)
    {
        depth++;
        parent = parent->parent;
    }
    return depth;
}

static int get_height_recursive(const ui_tree_node *node)
{
    if (!node)
        return 0;

    int max_height = 0;
    for (int i = 0; i < node->child_count; i++)
    {
        int child_height = get_height_recursive(node->children[i]);
        if (child_height > max_height)
        {
            max_height = child_height;
        }
    }

    return max_height + 1;
}

int ui_tree_node_get_height(const ui_tree_node *root)
{
    return get_height_recursive(root);
}

int ui_tree_node_is_descendant(const ui_tree_node *node, const ui_tree_node *ancestor)
{
    if (!node || !ancestor)
        return 0;

    ui_tree_node *parent = node->parent;
    while (parent)
    {
        if (parent == ancestor)
            return 1;
        parent = parent->parent;
    }
    return 0;
}

void ui_tree_node_set_user_data(ui_tree_node *node, void *user_data)
{
    if (node)
    {
        node->user_data = user_data;
    }
}

void *ui_tree_node_get_user_data(const ui_tree_node *node)
{
    return node ? node->user_data : NULL;
}

static ui_tree_node *clone_recursive(const ui_tree_node *node)
{
    if (!node)
        return NULL;

    ui_tree_node *new_node = ui_tree_node_create(node->name, node->rect, node->id);
    if (!new_node)
        return NULL;

    new_node->user_data = node->user_data; /* 注意：这里是浅拷贝用户数据 */

    /* 递归克隆子节点 */
    for (int i = 0; i < node->child_count; i++)
    {
        ui_tree_node *new_child = clone_recursive(node->children[i]);
        if (new_child)
        {
            ui_tree_node_add_child(new_node, new_child);
        }
    }

    return new_node;
}

ui_tree_node *ui_tree_node_clone(const ui_tree_node *node)
{
    return clone_recursive(node);
}

/* 辅助函数：计算树中节点总数 */
static int count_tree_nodes(const ui_tree_node *node)
{
    if (!node)
        return 0;

    int count = 1; /* 当前节点 */
    for (int i = 0; i < node->child_count; i++)
    {
        count += count_tree_nodes(node->children[i]);
    }

    return count;
}

/* 用于打印节点的回调函数 */
static void print_node_callback(const ui_tree_node *node, int depth, void *user_data)
{
    (void)user_data;

    /* 根据深度缩进 */
    for (int i = 0; i < depth; i++)
    {
        printf("  ");
    }

    /* 打印节点信息 */
    if (depth == 0)
    {
        printf("🌳 ");
    }
    else if (node->child_count > 0)
    {
        printf("├─ ");
    }
    else
    {
        printf("└─ ");
    }

    printf("%s (ID:%d) ",
           node->name ? node->name : "unnamed",
           node->id);

    printf("[%d,%d %dx%d]",
           node->rect.x, node->rect.y,
           node->rect.w, node->rect.h);

    if (node->child_count > 0)
    {
        printf(" (%d children)", node->child_count);
    }

    printf("\n");
}

void ui_tree_node_print_tree(const ui_tree_node *root)
{
    if (!root)
        return;

    printf("\n=== UI树结构 ===\n");
    ui_tree_node_traverse_dfs(root, print_node_callback, NULL);
    printf("================\n\n");
}

/* 从widget树初始化UI树节点结构 */
void init_child_recursive(widget_t *parent, ui_tree_node *parent_node, int32_t level)
{
    if (!parent || !parent_node)
        return;

    printf("%*s初始化层级 %d: %s\n", level * 2, "", level,
           parent_node->name ? parent_node->name : "unnamed");

    /* 获取直接子控件数量 */
    int32_t direct_children = widget_count_children(parent);
    printf("%*s  直接子控件数量: %d\n", level * 2, "", direct_children);

    if (direct_children <= 0)
    {
        return;
    }

    /* 预分配子节点数组空间 */
    if (!ensure_child_capacity(parent_node, direct_children))
    {
        printf("%*s  错误: 无法分配子节点数组空间\n", level * 2, "");
        return;
    }

    for (int32_t child_index = 0; child_index < direct_children; child_index++)
    {
        widget_t *child_widget = widget_get_child(parent, child_index);
        /* 创建子节点的矩形信息 */
        rect_t child_rect = {
            .x = child_widget->x,
            .y = child_widget->y,
            .w = child_widget->w,
            .h = child_widget->h};

        /* 创建UI树节点 */
        ui_tree_node *child_node = ui_tree_node_create(
            child_widget->name,
            child_rect,
            child_index /* 使用索引作为ID，可以根据需要调整 */
        );
        if (tk_str_eq(widget_get_type(child_widget), WIDGET_TYPE_LABEL) ||
            tk_str_eq(widget_get_type(child_widget), "shadow_label"))
        {
            value_t value;
            widget_get_style(child_widget, "normal:font_size", &value);
            child_node->text_size = value_int32(&value);
        }

        if (child_node)
        {
            /* 设置用户数据指向原始widget（可选） */
            child_node->user_data = child_widget;

            /* 直接添加到父节点的子节点数组中 */
            parent_node->children[parent_node->child_count++] = child_node;
            child_node->parent = parent_node;

            printf("%*s  └── 子节点 %d: %s [%d,%d,%d,%d]\n",
                   level * 2, "", child_index,
                   child_widget->name ? child_widget->name : "unnamed",
                   child_rect.x, child_rect.y, child_rect.w, child_rect.w);

            /* 递归初始化子节点的子节点 */
            init_child_recursive(child_widget, child_node, level + 1);
        }
    }

    printf("%*s  └── 成功添加 %d/%d 个子节点\n", level * 2, "", direct_children, direct_children);
}

/* 从根widget创建完整的UI树 */
ui_tree_node *ui_tree_node_create_from_widget_tree(widget_t *root_widget)
{
    if (!root_widget)
        return NULL;

    printf("开始构建UI树...\n");

    /* 创建根节点 */
    rect_t root_rect = {
        .x = root_widget->x,
        .y = root_widget->y,
        .w = root_widget->w,
        .w = root_widget->h};

    ui_tree_node *root_node = ui_tree_node_create(
        root_widget->name ? root_widget->name : "Root",
        root_rect,
        0 /* 根节点ID */
    );

    if (!root_node)
    {
        printf("创建根节点失败！\n");
        return NULL;
    }

    /* 设置用户数据指向原始widget */
    root_node->user_data = root_widget;

    printf("根节点: %s [%d,%d,%d,%d]\n",
           root_node->name ? root_node->name : "Root",
           root_rect.x, root_rect.y, root_rect.w, root_rect.h);

    /* 递归构建整个树 */
    init_child_recursive(root_widget, root_node, 1);

    printf("UI树构建完成！\n");
    printf("树高度: %d\n", ui_tree_node_get_height(root_node));
    printf("总节点数: %d\n", count_tree_nodes(root_node));

    return root_node;
}