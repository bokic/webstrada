import { Component } from '@angular/core';

interface MemoryRegion {
  name: string;
  used: number;
  max: number;
  unit: string;
}

interface ThreadRow {
  name: string;
  state: string;
  cpu: string;
}

@Component({
  selector: 'app-runtime',
  imports: [],
  templateUrl: './runtime.html',
  styleUrl: './runtime.css',
})
export class Runtime {
  protected readonly version = 'WebStrada v0.1.0 (LLVM JIT)';
  protected readonly built = '2026-08-01';

  protected readonly memory: MemoryRegion[] = [
    { name: 'Heap Used', used: 780, max: 2048, unit: 'MB' },
    { name: 'Template Cache', used: 41, max: 256, unit: 'MB' },
    { name: 'Query Cache', used: 12, max: 64, unit: 'MB' },
    { name: 'Worker Stack', used: 3, max: 32, unit: 'MB' },
  ];

  protected readonly threads: ThreadRow[] = [
    { name: 'Request Worker #1', state: 'Running', cpu: '0.4%' },
    { name: 'Request Worker #2', state: 'Waiting', cpu: '0.0%' },
    { name: 'Request Worker #3', state: 'Waiting', cpu: '0.0%' },
    { name: 'Template Cache Reaper', state: 'Waiting', cpu: '0.1%' },
    { name: 'Database Connection Pool', state: 'Running', cpu: '0.2%' },
    { name: 'Session Cleaner', state: 'Waiting', cpu: '0.0%' },
  ];
}
