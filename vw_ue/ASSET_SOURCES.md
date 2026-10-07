# 模型素材来源

本轮使用本机 UE 5.7 官方模板资源，不是 CC0 或开源资产；使用范围遵循 Epic 对 Unreal Engine 模板内容的许可。

- Content/Characters：Templates/TemplateResources/High/Characters/Content。Manny 骨骼角色、原生材质、无武器和步枪待机/移动/落下动画。
- Content/Weapons：Templates/TemplateResources/Standard/Weapons/Content。模板步枪与原生纹理材质。
- Content/Building：Templates/TemplateResources/Standard/Building/Content。建筑可视化网格、路灯、花坛、混凝土与金属材质。材质引用 UE 自带 DatasmithContent。

保留官方模板挂载路径，避免素材依赖断开。当前人物是 UE Manny 机器人角色，并非扫描真人。建筑采用官方建筑可视化模型；交互终端和射击靶仍保留功能原型。动作目前使用原生动画片段切换，尚未实现完整瞄准偏移、脚部 IK 和重装弹动作状态机。
- Content/ArchVis：Standard/ArchVis 官方模板完整建筑场景；复制其建筑网格及原始组合变换用于城镇。
