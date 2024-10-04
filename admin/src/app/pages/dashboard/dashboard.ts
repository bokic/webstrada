import { Component, inject, OnInit, signal } from '@angular/core';

import { AdminApiService } from '../../services/admin-api.service';

interface Stat {
  label: string;
  value: string;
  sub: string;
}

interface RequestRow {
  time: string;
  template: string;
  status: string;
  method: string;
  duration: string;
}

function formatTime(epoch: number): string {
  const d = new Date(epoch * 1000);
  return d.toTimeString().slice(0, 8);
}

function formatUptime(seconds: number): string {
  const d = Math.floor(seconds / 86400);
  const h = Math.floor((seconds % 86400) / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  if (d > 0) return `${d}d ${h}h ${m}m`;
  if (h > 0) return `${h}h ${m}m`;
  return `${m}m`;
}

function statusText(code: number): string {
  switch (code) {
    case 200: return '200 OK';
    case 204: return '204 No Content';
    case 302: return '302 Found';
    case 400: return '400 Bad Request';
    case 404: return '404 Not Found';
    case 500: return '500 Error';
    default: return String(code);
  }
}

@Component({
  selector: 'app-dashboard',
  imports: [],
  templateUrl: './dashboard.html',
  styleUrl: './dashboard.css',
})
export class Dashboard implements OnInit {
  private api = inject(AdminApiService);

  // Signals so the zoneless app re-renders when the async API response lands.
  protected readonly loaded = signal(false);
  protected readonly failed = signal(false);
  protected readonly stats = signal<Stat[]>([]);
  protected readonly recent = signal<RequestRow[]>([]);
  // Filter the recent-requests table server-side; on by default.
  protected readonly excludeAdmin = signal(true);

  ngOnInit() {
    // Fetch and show the server stats immediately on page load; the user can
    // refresh later with the Refresh button.
    this.refresh();
  }

  protected toggleAdminFilter() {
    this.excludeAdmin.update((v) => !v);
    this.refresh();
  }

  protected refresh() {
    this.failed.set(false);
    this.api.getServerInfo(this.excludeAdmin()).subscribe({
      next: (info) => {
        this.stats.set([
          { label: 'Server State', value: info.state, sub: info.version },
          { label: 'Uptime', value: formatUptime(info.uptimeSeconds), sub: 'since restart' },
          { label: 'Requests Served', value: info.requestsServed.toLocaleString(), sub: 'since last restart' },
          { label: 'Avg Response', value: `${info.avgResponseMs.toFixed(1)} ms`, sub: 'all requests' },
        ]);
        this.recent.set(
          info.recentRequests.map((r) => ({
            time: formatTime(r.time),
            template: r.template,
            status: statusText(r.status),
            method: r.method,
            duration: `${r.durationMs.toFixed(1)} ms`,
          })),
        );
        this.loaded.set(true);
      },
      error: (err) => {
        console.error('Failed to load server info:', err);
        this.loaded.set(true);
        this.failed.set(true);
      },
    });
  }
}
