import { Component } from '@angular/core';

interface LogFile {
  name: string;
  level: string;
  size: string;
  lastModified: string;
  entries: string;
}

interface LogLine {
  time: string;
  level: 'INFO' | 'WARN' | 'ERROR' | 'DEBUG';
  message: string;
}

@Component({
  selector: 'app-logs',
  imports: [],
  templateUrl: './logs.html',
  styleUrl: './logs.css',
})
export class Logs {
  protected readonly files: LogFile[] = [
    { name: 'webstrada.log', level: 'INFO', size: '1.2 MB', lastModified: '2026-08-10 14:32:18', entries: '4,201' },
    { name: 'exception.log', level: 'WARN', size: '320 KB', lastModified: '2026-08-10 13:58:02', entries: '89' },
    { name: 'query.log', level: 'DEBUG', size: '4.8 MB', lastModified: '2026-08-10 14:31:55', entries: '12,740' },
    { name: 'request.log', level: 'INFO', size: '2.1 MB', lastModified: '2026-08-10 14:32:07', entries: '9,308' },
    { name: 'startup.log', level: 'INFO', size: '18 KB', lastModified: '2026-08-07 09:12:44', entries: '41' },
  ];

  protected readonly lines: LogLine[] = [
    { time: '14:32:08', level: 'INFO', message: 'Request completed GET /index.cfm in 9 ms' },
    { time: '14:31:57', level: 'WARN', message: '404 for GET /api/orders.json' },
    { time: '14:31:48', level: 'INFO', message: 'Request completed GET /api/users/42.cfm in 21 ms' },
    { time: '14:31:40', level: 'ERROR', message: 'cfcatch: Type Application, message "Unable to find column id in query."' },
    { time: '14:30:22', level: 'INFO', message: 'Session scope created for client 192.168.1.42' },
    { time: '14:29:05', level: 'DEBUG', message: 'Query executed SELECT id,name FROM customers, 128 rows in 3 ms' },
    { time: '14:28:44', level: 'WARN', message: 'Memory usage above 80%, consider increasing heap size' },
  ];

  protected levelClass(level: string): string {
    return level.toLowerCase();
  }
}
