
## 🎮 High-Performance Swarm Simulation Engine

**`C++`** • **`Custom ECS`** • **`Spatial Partitioning`**

### *200,000 AI entities.*

**🔑 What Makes This Special:**
- ⚡ **Spatial Grid Hashing** for O(1) neighbor queries instead of O(n²) brute force
- 🎯 **Distance-Based LOD System** - entities far from camera are not taking up resources
- 🧠 **Data-Oriented Design** - AoS → SoA refactor for better cache perfromance


**💡 Technical Deep Dive:**
- Custom entity component system built from scratch
- Uniform grid divides world into cells - only check neighbors in same/adjacent cells
- Hot loop processes contiguous memory chunks (SIMD-friendly)
- AI update budget allocated by distance: close entities get full logic, distant ones get sparse updates

<br>

[![YouTube](https://img.shields.io/badge/YouTube-Watch%20Demo-red?style=for-the-badge&logo=youtube&logoColor=white)](https://www.youtube.com/watch?v=4R8rz6CtQNw) 
