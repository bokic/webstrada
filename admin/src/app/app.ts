import { Component } from '@angular/core';
import { RouterLink, RouterLinkActive, RouterOutlet } from '@angular/router';

import { AppIcon } from './icons/icon';

export interface NavItem {
  label: string;
  route: string;
  icon: string;
}

export interface NavSection {
  title: string;
  items: NavItem[];
}

@Component({
  selector: 'app-root',
  imports: [RouterOutlet, RouterLink, RouterLinkActive, AppIcon],
  templateUrl: './app.html',
  styleUrl: './app.css',
})
export class App {
  protected readonly brand = 'WebStrada';
  protected readonly subtitle = 'Administrator';
  protected readonly nav: NavSection[] = [
    {
      title: '',
      items: [{ label: 'Dashboard', route: '/', icon: 'dashboard' }],
    },
    {
      title: 'Data & Services',
      items: [
        { label: 'Data Sources', route: '/datasources', icon: 'database' },
        { label: 'Cache', route: '/cache', icon: 'cache' },
      ],
    },
    {
      title: 'Server',
      items: [
        { label: 'General Settings', route: '/settings', icon: 'settings' },
        { label: 'Debugging & Logging', route: '/logs', icon: 'logs' },
        { label: 'Execution Tracing', route: '/tracing', icon: 'tracing' },
        { label: 'Memory & Runtime', route: '/runtime', icon: 'runtime' },
      ],
    },
    {
      title: 'Security',
      items: [{ label: 'Users & Roles', route: '/users', icon: 'users' }],
    },
  ];
}
