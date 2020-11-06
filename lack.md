状态机：

- 各状态类之间互相依赖很严重，耦合度很高，可复用性差
- 结构不灵活，可扩展性差，难以脚本化/可视化
- 无法应对复杂的AI需求

行为树：

- 树状结构
- 从根节点开始自顶向下执行，最终执行节点为对应行为
- 易脚本化/可视化
- 逻辑和状态数据低耦合，任何节点写好可复用节点
- 可迅速而便捷的组织复杂的行为决策
- 可配置，把工作交给designer

行为树缺点：

- 每一帧都从root开始，有可能访问到所有的节点，对状态机消耗更多的cpu
