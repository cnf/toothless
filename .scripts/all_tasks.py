import * as vscode from 'vscode';

let rakePromise: Thenable<vscode.Task[]> | undefined = undefined;
const taskProvider = vscode.tasks.registerTaskProvider('rake', {
  provideTasks: () => {
    if (!rakePromise) {
      rakePromise = getRakeTasks();
    }
    return rakePromise;
  },
  resolveTask(_task: vscode.Task): vscode.Task | undefined {
    const task = _task.definition.task;
    // A Rake task consists of a task and an optional file as specified in RakeTaskDefinition
    // Make sure that this looks like a Rake task by checking that there is a task.
    if (task) {
      // resolveTask requires that the same definition object be used.
      const definition: RakeTaskDefinition = <any>_task.definition;
      return new vscode.Task(
        definition,
        _task.scope ?? vscode.TaskScope.Workspace,
        definition.task,
        'rake',
        new vscode.ShellExecution(`rake ${definition.task}`)
      );
    }
    return undefined;
  }
});
