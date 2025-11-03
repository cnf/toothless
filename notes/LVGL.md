📋 ARCHITECTURAL CONCERNS
1. Global State in Display Class
Static variables make testing difficult and create hidden dependencies.

2. Tight Coupling
Screens directly access Display static methods instead of using dependency injection.

3. No Resource Management
No RAII for LVGL objects - manual cleanup is error-prone.

