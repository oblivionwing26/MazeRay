MazeRay v1.0
=========================================
*Overview*

A first-person shooter maze game inspired by classic Wolfenstein 3D. Navigate through procedurally generated mazes, collect keys, and find the exit while battling enemies using a custom-built raycasting engine.

*Controls*

Movement: Arrow keys or WASD
Attack: Spacebar
Pause/Resume: P key
Exit: ESC key
Restart after victory/defeat: R key

*Objective*

Find all 3 keys scattered throughout the maze, then locate the exit door while avoiding or defeating enemies. The maze layout is procedurally generated each time you play, providing a unique experience with every run.

*Game Elements*

Keys: Collect all 3 to unlock the exit door
Enemies: Hostile creatures that will chase and attack you
Exit Door: Only accessible after collecting all keys

*Technical Details*

Graphics Engine: Custom raycasting implementation using Raylib
Maze Generation: Procedural generation using modified DFS algorithm
Rendering: Column-based raycasting with z-buffer for sprite handling
Optimization: Grid-based collision detection and efficient texture management
No installation required - run the executable directly