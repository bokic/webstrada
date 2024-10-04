import { Routes } from '@angular/router';

export const routes: Routes = [
  {
    path: '',
    title: 'Dashboard - WebStrada Administrator',
    loadComponent: () =>
      import('./pages/dashboard/dashboard').then((m) => m.Dashboard),
  },
  {
    path: 'datasources',
    title: 'Data Sources - WebStrada Administrator',
    loadComponent: () =>
      import('./pages/datasources/datasources').then((m) => m.Datasources),
  },
  {
    path: 'cache',
    title: 'Cache - WebStrada Administrator',
    loadComponent: () =>
      import('./pages/cache/cache').then((m) => m.Cache),
  },
  {
    path: 'settings',
    title: 'General Settings - WebStrada Administrator',
    loadComponent: () =>
      import('./pages/settings/settings').then((m) => m.Settings),
  },
  {
    path: 'logs',
    title: 'Debugging & Logging - WebStrada Administrator',
    loadComponent: () =>
      import('./pages/logs/logs').then((m) => m.Logs),
  },
  {
    path: 'runtime',
    title: 'Memory & Runtime - WebStrada Administrator',
    loadComponent: () =>
      import('./pages/runtime/runtime').then((m) => m.Runtime),
  },
  {
    path: 'users',
    title: 'Users & Roles - WebStrada Administrator',
    loadComponent: () =>
      import('./pages/users/users').then((m) => m.Users),
  },
  {
    path: '**',
    redirectTo: '',
  },
];
