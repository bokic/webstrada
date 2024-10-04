import { Component } from '@angular/core';

interface User {
  username: string;
  role: string;
  email: string;
  status: 'Active' | 'Disabled';
  lastLogin: string;
}

@Component({
  selector: 'app-users',
  imports: [],
  templateUrl: './users.html',
  styleUrl: './users.css',
})
export class Users {
  protected readonly users: User[] = [
    { username: 'admin', role: 'Administrator', email: 'admin@webstrada.dev', status: 'Active', lastLogin: '2026-08-10 08:12' },
    { username: 'boris', role: 'Server Manager', email: 'boris@webstrada.dev', status: 'Active', lastLogin: '2026-08-10 14:30' },
    { username: 'deploy', role: 'Deployment', email: 'ci@webstrada.dev', status: 'Active', lastLogin: '2026-08-10 13:05' },
    { username: 'qa-reader', role: 'Monitoring', email: 'qa@webstrada.dev', status: 'Disabled', lastLogin: '2026-07-28 16:44' },
    { username: 'guest', role: 'Read Only', email: 'guest@webstrada.dev', status: 'Active', lastLogin: '2026-08-09 11:19' },
  ];
}
