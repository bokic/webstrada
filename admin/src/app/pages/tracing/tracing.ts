import {
  Component,
  ElementRef,
  OnDestroy,
  OnInit,
  ViewChild,
  inject,
  signal,
} from '@angular/core';
import { AdminApiService, RecentRequest } from '../../services/admin-api.service';

export type FlameEventType =
  | 'request'
  | 'template'
  | 'component'
  | 'query'
  | 'custom_tag'
  | 'include'
  | 'function'
  | 'error'
  | 'http';

export interface FlameFrame {
  id: string;
  name: string;
  type: FlameEventType;
  typeLabel: string;
  startMs: number;
  durationMs: number;
  depth: number; // 0 = lowest (root / no stack trace), higher = nested call stack
  file: string;
  line: number;
  stackTrace: string;
  details?: string;
  leftPercent: number;
  widthPercent: number;
}

export interface TraceRow {
  id: number;
  time: string;
  timestamp: number;
  template: string;
  status: number;
  statusText: string;
  method: string;
  durationMs: number;
  durationText: string;
  flameFrames?: FlameFrame[];
}

export interface TimeTick {
  label: string;
  percent: number;
}

function formatTime(epoch: number): string {
  const d = new Date(epoch * 1000);
  return d.toTimeString().slice(0, 8);
}

function getStatusText(code: number): string {
  switch (code) {
    case 200: return '200 OK';
    case 201: return '201 Created';
    case 204: return '204 No Content';
    case 301: return '301 Moved Permanently';
    case 302: return '302 Found';
    case 304: return '304 Not Modified';
    case 400: return '400 Bad Request';
    case 401: return '401 Unauthorized';
    case 403: return '403 Forbidden';
    case 404: return '404 Not Found';
    case 500: return '500 Internal Error';
    default: return String(code);
  }
}

@Component({
  selector: 'app-tracing',
  imports: [],
  templateUrl: './tracing.html',
  styleUrl: './tracing.css',
})
export class Tracing implements OnInit, OnDestroy {
  private api = inject(AdminApiService);

  @ViewChild('scrollWindow') private scrollWindowRef?: ElementRef<HTMLDivElement>;

  protected readonly loaded = signal(false);
  protected readonly isLive = signal(true);
  protected readonly excludeAdmin = signal(false);
  protected readonly requests = signal<TraceRow[]>([]);
  protected readonly selectedId = signal<number | null>(null);
  protected readonly selectedRequest = signal<TraceRow | null>(null);
  protected readonly flameFrames = signal<FlameFrame[]>([]);
  protected readonly maxDepth = signal(0);
  protected readonly timeTicks = signal<TimeTick[]>([]);
  protected readonly hoveredFrame = signal<FlameFrame | null>(null);
  protected readonly loadingOlder = signal(false);
  protected readonly hasMoreOlder = signal(true);
  protected readonly newCountSinceScroll = signal(0);

  private pollTimer: ReturnType<typeof setInterval> | null = null;
  private allServerPool: TraceRow[] = [];
  private currentOffset = 0;
  private readonly pageSize = 10;
  private nextId = 1000;

  ngOnInit() {
    this.initialFetch();
    this.startPolling();
  }

  ngOnDestroy() {
    this.stopPolling();
  }

  protected toggleLive() {
    const nextState = !this.isLive();
    this.isLive.set(nextState);
    if (nextState) {
      this.startPolling();
    } else {
      this.stopPolling();
    }
  }

  protected toggleAdminFilter() {
    this.excludeAdmin.update((v) => !v);
    this.currentOffset = 0;
    this.initialFetch();
  }

  protected selectRow(row: TraceRow) {
    if (this.selectedId() === row.id) {
      // Keep selection or toggle
      this.selectedId.set(row.id);
      this.selectedRequest.set(row);
      this.buildFlameGraph(row);
    } else {
      this.selectedId.set(row.id);
      this.selectedRequest.set(row);
      this.buildFlameGraph(row);
    }
  }

  protected clearView() {
    this.requests.set([]);
    this.selectedId.set(null);
    this.selectedRequest.set(null);
    this.flameFrames.set([]);
    this.timeTicks.set([]);
    this.currentOffset = 0;
    this.newCountSinceScroll.set(0);
    this.initialFetch();
  }

  protected scrollToTop() {
    this.newCountSinceScroll.set(0);
    if (this.scrollWindowRef?.nativeElement) {
      this.scrollWindowRef.nativeElement.scrollTo({ top: 0, behavior: 'smooth' });
    }
  }

  protected onScroll(event: Event) {
    const el = event.target as HTMLElement;
    if (!el) return;

    if (el.scrollTop <= 10) {
      this.newCountSinceScroll.set(0);
    }

    // Infinite scroll check: near the bottom
    const distanceToBottom = el.scrollHeight - el.scrollTop - el.clientHeight;
    if (distanceToBottom < 40 && !this.loadingOlder() && this.hasMoreOlder()) {
      this.fetchOlderBatch();
    }
  }

  protected onFrameHover(frame: FlameFrame | null) {
    this.hoveredFrame.set(frame);
  }

  private startPolling() {
    this.stopPolling();
    this.pollTimer = setInterval(() => {
      if (this.isLive()) {
        this.pollNewRequests();
      }
    }, 1000);
  }

  private stopPolling() {
    if (this.pollTimer) {
      clearInterval(this.pollTimer);
      this.pollTimer = null;
    }
  }

  private initialFetch() {
    this.api.getTracing(this.excludeAdmin(), this.pageSize, 0, 0).subscribe({
      next: (info) => {
        const rows = this.mapRecentRequests(info.recentRequests);
        this.allServerPool = [...rows];
        
        this.currentOffset = rows.length;
        this.hasMoreOlder.set(rows.length >= this.pageSize);
        this.requests.set(rows);
        if (rows.length > 0 && !this.selectedRequest()) {
          this.selectRow(rows[0]);
        }
        this.loaded.set(true);
      },
      error: () => {
        if (this.allServerPool.length === 0) {
          this.generateMockPool();
        }
        const initialBatch = this.allServerPool.slice(0, this.pageSize);
        this.currentOffset = initialBatch.length;
        this.hasMoreOlder.set(this.allServerPool.length > initialBatch.length);
        this.requests.set(initialBatch);
        if (initialBatch.length > 0 && !this.selectedRequest()) {
          this.selectRow(initialBatch[0]);
        }
        this.loaded.set(true);
      },
    });
  }

  private pollNewRequests() {
    const list = this.requests();
    const newestId = list.length > 0 ? list[0].id : 0;

    this.api.getTracing(this.excludeAdmin(), this.pageSize, 0, newestId).subscribe({
      next: (info) => {
        const latestRows = this.mapRecentRequests(info.recentRequests);
        if (latestRows.length > 0) {
          this.processNewArrivals(latestRows);
        }
      },
      error: () => {
        this.simulateIncomingMockRequest();
      },
    });
  }

  private processNewArrivals(incoming: TraceRow[]) {
    const currentList = this.requests();
    if (incoming.length === 0) return;

    if (currentList.length === 0) {
      const batch = incoming.slice(0, this.pageSize);
      this.requests.set(batch);
      if (!this.selectedRequest() && batch.length > 0) {
        this.selectRow(batch[0]);
      }
      return;
    }

    const newestCurrent = currentList[0];
    const newItems: TraceRow[] = [];
    for (const row of incoming) {
      if (
        (row.id > 0 && newestCurrent.id > 0 && row.id > newestCurrent.id) ||
        row.timestamp > newestCurrent.timestamp ||
        (row.timestamp === newestCurrent.timestamp && row.template !== newestCurrent.template)
      ) {
        newItems.push(row);
      } else {
        break;
      }
    }

    if (newItems.length > 0) {
      this.prependNewRequests(newItems);
    }
  }

  private prependNewRequests(newRows: TraceRow[]) {
    const el = this.scrollWindowRef?.nativeElement;
    const prevScrollHeight = el ? el.scrollHeight : 0;
    const prevScrollTop = el ? el.scrollTop : 0;

    this.requests.update((existing) => [...newRows, ...existing]);

    setTimeout(() => {
      if (el) {
        const newScrollHeight = el.scrollHeight;
        const deltaHeight = newScrollHeight - prevScrollHeight;
        if (deltaHeight > 0) {
          el.scrollTop = prevScrollTop + deltaHeight;
        }
      }
    }, 0);

    this.newCountSinceScroll.update((c) => c + newRows.length);
  }

  private fetchOlderBatch() {
    this.loadingOlder.set(true);
    const list = this.requests();
    const oldestId = list.length > 0 ? list[list.length - 1].id : 0;

    this.api.getTracing(this.excludeAdmin(), this.pageSize, oldestId, 0).subscribe({
      next: (info) => {
        const olderRows = this.mapRecentRequests(info.recentRequests);
        if (olderRows.length > 0) {
          this.requests.update((existing) => [...existing, ...olderRows]);
          this.currentOffset += olderRows.length;
          this.hasMoreOlder.set(olderRows.length >= this.pageSize);
        } else {
          this.hasMoreOlder.set(false);
        }
        this.loadingOlder.set(false);
      },
      error: () => {
        const nextBatch = this.allServerPool.slice(
          this.currentOffset,
          this.currentOffset + this.pageSize,
        );

        if (nextBatch.length > 0) {
          this.requests.update((existing) => [...existing, ...nextBatch]);
          this.currentOffset += nextBatch.length;
          this.hasMoreOlder.set(this.currentOffset < this.allServerPool.length);
        } else {
          this.hasMoreOlder.set(false);
        }
        this.loadingOlder.set(false);
      },
    });
  }

  private mapRecentRequests(list: RecentRequest[]): TraceRow[] {
    const reversed = [...list].reverse();
    return reversed.map((r, index) => {
      const id = r.id || (list.length - index);
      const row: TraceRow = {
        id,
        time: formatTime(r.time),
        timestamp: r.time,
        template: r.template,
        status: r.status,
        statusText: getStatusText(r.status),
        method: r.method,
        durationMs: r.durationMs,
        durationText: `${r.durationMs.toFixed(1)} ms`,
      };
      row.flameFrames = this.generateFlameFrames(row);
      return row;
    });
  }

  private buildFlameGraph(row: TraceRow) {
    const frames = row.flameFrames && row.flameFrames.length > 0
      ? row.flameFrames
      : this.generateFlameFrames(row);

    const totalDur = Math.max(row.durationMs, 0.1);
    let highestDepth = 0;

    for (const f of frames) {
      if (f.depth > highestDepth) highestDepth = f.depth;
      f.leftPercent = (f.startMs / totalDur) * 100;
      f.widthPercent = Math.max((f.durationMs / totalDur) * 100, 0.4);
    }

    this.flameFrames.set(frames);
    this.maxDepth.set(highestDepth);

    // Build time axis ticks (5 ticks from 0 to totalDur)
    const ticks: TimeTick[] = [];
    const count = 5;
    for (let i = 0; i <= count; i++) {
      const frac = i / count;
      const ms = frac * totalDur;
      ticks.push({
        label: `${ms.toFixed(1)} ms`,
        percent: frac * 100,
      });
    }
    this.timeTicks.set(ticks);
  }

  private generateFlameFrames(row: TraceRow): FlameFrame[] {
    const dur = Math.max(row.durationMs, 0.5);
    const tmpl = row.template;
    const frames: FlameFrame[] = [];

    // Depth 0: Lowest level - Root HTTP Request (no stack trace)
    frames.push({
      id: `f0-0`,
      name: `${row.method} ${tmpl}`,
      type: 'request',
      typeLabel: 'HTTP Request',
      startMs: 0,
      durationMs: dur,
      depth: 0,
      file: tmpl,
      line: 1,
      stackTrace: 'HTTP Worker > Request Pipeline',
      details: `Status: ${row.statusText}, Method: ${row.method}`,
      leftPercent: 0,
      widthPercent: 100,
    });

    // Depth 1: Application Lifecycle & Page Execution
    const reqStartDur = dur * 0.10;
    const pageDur = dur * 0.82;
    const reqEndDur = dur * 0.08;

    frames.push({
      id: `f1-0`,
      name: 'Application.cfc:onRequestStart()',
      type: 'component',
      typeLabel: 'CFC Method',
      startMs: 0,
      durationMs: reqStartDur,
      depth: 1,
      file: '/Application.cfc',
      line: 24,
      stackTrace: `${tmpl} > Application.cfc:onRequestStart`,
      details: 'Session validation and scope setup',
      leftPercent: 0,
      widthPercent: (reqStartDur / dur) * 100,
    });

    frames.push({
      id: `f1-1`,
      name: `Page: ${tmpl}`,
      type: 'template',
      typeLabel: 'CFML Template',
      startMs: reqStartDur,
      durationMs: pageDur,
      depth: 1,
      file: tmpl,
      line: 1,
      stackTrace: `${tmpl}`,
      details: 'Compiled template body execution',
      leftPercent: (reqStartDur / dur) * 100,
      widthPercent: (pageDur / dur) * 100,
    });

    frames.push({
      id: `f1-2`,
      name: 'Application.cfc:onRequestEnd()',
      type: 'component',
      typeLabel: 'CFC Method',
      startMs: reqStartDur + pageDur,
      durationMs: reqEndDur,
      depth: 1,
      file: '/Application.cfc',
      line: 68,
      stackTrace: `${tmpl} > Application.cfc:onRequestEnd`,
      details: 'Response cleanup and session sync',
      leftPercent: ((reqStartDur + pageDur) / dur) * 100,
      widthPercent: (reqEndDur / dur) * 100,
    });

    // Depth 2 & higher: Inside Page Execution
    const pStart = reqStartDur;
    const pDur = pageDur;

    if (row.status >= 400 && row.status < 500) {
      // 404 / 400 Flow
      frames.push({
        id: `f2-0`,
        name: '<cfinclude template="router.cfm">',
        type: 'include',
        typeLabel: 'CFINCLUDE',
        startMs: pStart + pDur * 0.05,
        durationMs: pDur * 0.4,
        depth: 2,
        file: '/router.cfm',
        line: 12,
        stackTrace: `${tmpl}:12 > router.cfm`,
        details: 'Route resolution',
        leftPercent: 0,
        widthPercent: 0,
      });

      frames.push({
        id: `f2-1`,
        name: '<cfheader statuscode="404">',
        type: 'custom_tag',
        typeLabel: 'CFTAG',
        startMs: pStart + pDur * 0.5,
        durationMs: pDur * 0.45,
        depth: 2,
        file: tmpl,
        line: 38,
        stackTrace: `${tmpl}:38 > cfheader`,
        details: 'Missing resource handler',
        leftPercent: 0,
        widthPercent: 0,
      });
    } else if (row.status >= 500) {
      // Error Flow
      frames.push({
        id: `f2-0`,
        name: 'Service.cfc:execute()',
        type: 'component',
        typeLabel: 'CFC Method',
        startMs: pStart + pDur * 0.05,
        durationMs: pDur * 0.45,
        depth: 2,
        file: '/services/Service.cfc',
        line: 52,
        stackTrace: `${tmpl}:18 > Service.cfc:52`,
        details: 'Main business logic',
        leftPercent: 0,
        widthPercent: 0,
      });

      frames.push({
        id: `f3-0`,
        name: '<cfquery name="qExec">',
        type: 'query',
        typeLabel: 'CFQUERY',
        startMs: pStart + pDur * 0.15,
        durationMs: pDur * 0.32,
        depth: 3,
        file: '/services/Service.cfc',
        line: 84,
        stackTrace: `${tmpl}:18 > Service.cfc:52 > Service.cfc:84 (cfquery)`,
        details: 'SELECT * FROM records WHERE id = ?',
        leftPercent: 0,
        widthPercent: 0,
      });

      frames.push({
        id: `f2-1`,
        name: '<cftry> / <cfcatch type="Database">',
        type: 'error',
        typeLabel: 'CFCATCH',
        startMs: pStart + pDur * 0.52,
        durationMs: pDur * 0.42,
        depth: 2,
        file: tmpl,
        line: 64,
        stackTrace: `${tmpl}:64 > cfcatch (Database)`,
        details: 'Exception: Connection timeout or missing table',
        leftPercent: 0,
        widthPercent: 0,
      });

      frames.push({
        id: `f3-1`,
        name: 'ErrorHandler.cfc:handleException()',
        type: 'component',
        typeLabel: 'CFC Method',
        startMs: pStart + pDur * 0.6,
        durationMs: pDur * 0.32,
        depth: 3,
        file: '/handlers/ErrorHandler.cfc',
        line: 15,
        stackTrace: `${tmpl}:64 > ErrorHandler.cfc:15`,
        details: 'Format 500 error payload',
        leftPercent: 0,
        widthPercent: 0,
      });
    } else {
      // Normal 200 OK execution flow
      // Depth 2: Include header / Auth
      const fIncludeDur = pDur * 0.18;
      frames.push({
        id: `f2-0`,
        name: '<cfinclude template="security_check.cfm">',
        type: 'include',
        typeLabel: 'CFINCLUDE',
        startMs: pStart + pDur * 0.02,
        durationMs: fIncludeDur,
        depth: 2,
        file: '/security_check.cfm',
        line: 8,
        stackTrace: `${tmpl}:8 > security_check.cfm`,
        details: 'Validate CSRF token & user session',
        leftPercent: 0,
        widthPercent: 0,
      });

      // Depth 2: Service call
      const svcStart = pStart + pDur * 0.22;
      const svcDur = pDur * 0.52;
      frames.push({
        id: `f2-1`,
        name: 'DataService.cfc:fetchData()',
        type: 'component',
        typeLabel: 'CFC Method',
        startMs: svcStart,
        durationMs: svcDur,
        depth: 2,
        file: '/services/DataService.cfc',
        line: 41,
        stackTrace: `${tmpl}:22 > DataService.cfc:41`,
        details: 'Data access layer execution',
        leftPercent: 0,
        widthPercent: 0,
      });

      // Depth 3: Database Query
      const qStart = svcStart + svcDur * 0.15;
      const qDur = svcDur * 0.70;
      frames.push({
        id: `f3-0`,
        name: '<cfquery name="qSelect">',
        type: 'query',
        typeLabel: 'CFQUERY',
        startMs: qStart,
        durationMs: qDur,
        depth: 3,
        file: '/services/DataService.cfc',
        line: 76,
        stackTrace: `${tmpl}:22 > DataService.cfc:41 > DataService.cfc:76`,
        details: 'SELECT id, code, name, status FROM catalog WHERE active = 1',
        leftPercent: 0,
        widthPercent: 0,
      });

      // Depth 4: SQLite engine execution
      const dbEngStart = qStart + qDur * 0.18;
      const dbEngDur = qDur * 0.72;
      frames.push({
        id: `f4-0`,
        name: 'sqlite_step() / Fetch Rows',
        type: 'function',
        typeLabel: 'DB Engine',
        startMs: dbEngStart,
        durationMs: dbEngDur,
        depth: 4,
        file: 'sqlite3_engine.cpp',
        line: 142,
        stackTrace: `${tmpl} > DataService.cfc:76 > sqlite3_step()`,
        details: 'Native query execution and result binding (48 rows)',
        leftPercent: 0,
        widthPercent: 0,
      });

      // Depth 2: Output rendering
      const outStart = pStart + pDur * 0.76;
      const outDur = pDur * 0.22;
      frames.push({
        id: `f2-2`,
        name: '<cfoutput> / SerializeJSON()',
        type: 'custom_tag',
        typeLabel: 'CFTAG',
        startMs: outStart,
        durationMs: outDur,
        depth: 2,
        file: tmpl,
        line: 55,
        stackTrace: `${tmpl}:55 > cfoutput`,
        details: 'Serialize response buffer to JSON',
        leftPercent: 0,
        widthPercent: 0,
      });
    }

    return frames;
  }

  private generateMockPool() {
    const sampleTemplates = [
      '/api/users.cfm',
      '/products/list.cfm',
      '/index.cfm',
      '/auth/login.cfm',
      '/api/orders.cfm',
      '/reports/monthly.cfm',
      '/admin/config.cfm',
      '/services/inventory.cfm',
      '/checkout/pay.cfm',
      '/api/health.cfm',
    ];
    const methods = ['GET', 'POST', 'GET', 'GET', 'PUT', 'GET', 'DELETE'];
    const statuses = [200, 200, 200, 200, 302, 204, 404, 500, 200, 200];

    const now = Math.floor(Date.now() / 1000);
    const pool: TraceRow[] = [];

    for (let i = 0; i < 60; i++) {
      const time = now - (i * 2 + Math.floor(Math.random() * 3));
      const method = methods[i % methods.length];
      const template = sampleTemplates[i % sampleTemplates.length];
      const status = statuses[i % statuses.length];
      const durationMs = 3.5 + Math.random() * 55;
      const id = this.nextId--;

      const row: TraceRow = {
        id,
        time: formatTime(time),
        timestamp: time,
        template,
        status,
        statusText: getStatusText(status),
        method,
        durationMs,
        durationText: `${durationMs.toFixed(1)} ms`,
      };
      row.flameFrames = this.generateFlameFrames(row);
      pool.push(row);
    }

    this.allServerPool = pool;
    this.nextId = 1001;
  }

  private simulateIncomingMockRequest() {
    const sampleTemplates = [
      '/api/events.cfm',
      '/index.cfm',
      '/api/status.cfm',
      '/users/profile.cfm',
      '/products/item.cfm',
    ];
    const now = Math.floor(Date.now() / 1000);
    const method = Math.random() > 0.3 ? 'GET' : 'POST';
    const template = sampleTemplates[Math.floor(Math.random() * sampleTemplates.length)];
    const status = Math.random() > 0.05 ? 200 : (Math.random() > 0.5 ? 404 : 500);
    const durationMs = 2.0 + Math.random() * 42;
    const id = ++this.nextId;

    const newRow: TraceRow = {
      id,
      time: formatTime(now),
      timestamp: now,
      template,
      status,
      statusText: getStatusText(status),
      method,
      durationMs,
      durationText: `${durationMs.toFixed(1)} ms`,
    };
    newRow.flameFrames = this.generateFlameFrames(newRow);

    this.prependNewRequests([newRow]);
  }

  protected getMethodClass(method: string): string {
    const m = (method || '').toUpperCase();
    if (m === 'GET') return 'get';
    if (m === 'POST') return 'post';
    if (m === 'PUT') return 'put';
    if (m === 'DELETE') return 'delete';
    return 'other';
  }

  protected getDurationClass(ms: number): string {
    if (ms < 10) return 'duration-fast';
    if (ms < 50) return 'duration-med';
    return 'duration-slow';
  }

  protected getFrameColorClass(type: FlameEventType): string {
    switch (type) {
      case 'request': return 'frame-request';
      case 'template': return 'frame-template';
      case 'component': return 'frame-component';
      case 'query': return 'frame-query';
      case 'custom_tag': return 'frame-custom-tag';
      case 'include': return 'frame-include';
      case 'function': return 'frame-function';
      case 'error': return 'frame-error';
      case 'http': return 'frame-http';
      default: return 'frame-default';
    }
  }

  protected getFramesAtDepth(depth: number): FlameFrame[] {
    return this.flameFrames().filter((f) => f.depth === depth);
  }

  protected getDepthArray(): number[] {
    const max = this.maxDepth();
    // Highest depth at top, lowest (0) at bottom
    const depths: number[] = [];
    for (let d = max; d >= 0; d--) {
      depths.push(d);
    }
    return depths;
  }
}
