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
  | 'http'
  | 'parser'
  | 'compiler';

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
  protected readonly maxRows = 1000;
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

    this.requests.update((existing) => {
      const combined = [...newRows, ...existing];
      if (combined.length > this.maxRows) {
        // Drop oldest excess rows from the end of the table
        return combined.slice(0, this.maxRows);
      }
      return combined;
    });

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
    const list = this.requests();
    if (list.length >= this.maxRows) {
      this.hasMoreOlder.set(false);
      this.loadingOlder.set(false);
      return;
    }

    this.loadingOlder.set(true);
    const oldestId = list.length > 0 ? list[list.length - 1].id : 0;

    this.api.getTracing(this.excludeAdmin(), this.pageSize, oldestId, 0).subscribe({
      next: (info) => {
        const olderRows = this.mapRecentRequests(info.recentRequests);
        if (olderRows.length > 0) {
          this.requests.update((existing) => {
            const available = this.maxRows - existing.length;
            const toAdd = olderRows.slice(0, Math.max(0, available));
            return [...existing, ...toAdd];
          });
          this.currentOffset += olderRows.length;
          const currentTotal = this.requests().length;
          this.hasMoreOlder.set(olderRows.length >= this.pageSize && currentTotal < this.maxRows);
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
          this.requests.update((existing) => {
            const available = this.maxRows - existing.length;
            const toAdd = nextBatch.slice(0, Math.max(0, available));
            return [...existing, ...toAdd];
          });
          this.currentOffset += nextBatch.length;
          const currentTotal = this.requests().length;
          this.hasMoreOlder.set(this.currentOffset < this.allServerPool.length && currentTotal < this.maxRows);
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
    const totalDur = Math.max(row.durationMs, 0.1);
    this.updateTimeTicks(totalDur);

    // Fetch real SQLite line execution steps from backend profiler store
    this.api.getRequestTrace(row.id).subscribe({
      next: (details) => {
        if (details.steps && details.steps.length > 0) {
          const realFrames = this.mapSqliteStepsToFlameFrames(row, details.steps);
          this.applyFlameFrames(row, realFrames);
        } else {
          // Fallback to modeled frames if lineExecutionTrace was not active for this request
          const fallbackFrames = this.generateFlameFrames(row);
          this.applyFlameFrames(row, fallbackFrames);
        }
      },
      error: () => {
        const fallbackFrames = this.generateFlameFrames(row);
        this.applyFlameFrames(row, fallbackFrames);
      },
    });
  }

  private updateTimeTicks(totalDur: number) {
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

  private applyFlameFrames(row: TraceRow, frames: FlameFrame[]) {
    const totalDur = Math.max(row.durationMs, 0.1);
    let highestDepth = 0;

    for (const f of frames) {
      if (f.depth > highestDepth) highestDepth = f.depth;
      f.leftPercent = (f.startMs / totalDur) * 100;
      f.widthPercent = Math.max((f.durationMs / totalDur) * 100, 0.4);
    }

    this.flameFrames.set(frames);
    this.maxDepth.set(highestDepth);
  }

  private mapSqliteStepsToFlameFrames(row: TraceRow, steps: import('../../services/admin-api.service').TraceStepDto[]): FlameFrame[] {
    const totalDur = Math.max(row.durationMs, 0.1);
    const tmpl = row.template;
    const frames: FlameFrame[] = [];

    // Root Frame (Depth 0 - no stack trace)
    frames.push({
      id: `real-root-${row.id}`,
      name: `${row.method} ${tmpl}`,
      type: 'request',
      typeLabel: 'HTTP Request',
      startMs: 0,
      durationMs: totalDur,
      depth: 0,
      file: tmpl,
      line: 1,
      stackTrace: 'HTTP Worker > Request Pipeline',
      details: `Status: ${row.statusText}, Total Duration: ${totalDur.toFixed(2)} ms`,
      leftPercent: 0,
      widthPercent: 100,
    });

    let index = 0;
    const entryStack: { id: string; type: string; path: string; function: string; startMs: number; depth: number; stackTrace: string }[] = [];
    const parseStack: { id: string; path: string; startMs: number; depth: number }[] = [];
    const compileStack: { id: string; path: string; startMs: number; depth: number }[] = [];

    for (const s of steps) {
      const ts = s.timestampMs ?? s.elapsedMs ?? 0;
      const dur = Math.max(s.durationMs ?? s.deltaMs ?? 0, 0.001);
      const stepStartMs = Math.max(0, ts - dur);
      const stepEndMs = ts;

      // Determine base depth from stack trace hierarchy
      let stackDepth = 1;
      if (s.stackTrace && s.stackTrace.length > 0) {
        const countPipes = (s.stackTrace.match(/\|/g) || []).length;
        const countArrows = (s.stackTrace.match(/>/g) || []).length;
        stackDepth = 1 + Math.max(countPipes, countArrows);
      }

      const fileName = s.path ? s.path.split('/').pop() || s.path : tmpl;

      if (s.type === 'PARSE_START') {
        parseStack.push({
          id: `parse-${row.id}-${index++}`,
          path: s.path,
          startMs: ts,
          depth: stackDepth,
        });
      } else if (s.type === 'PARSE_END') {
        let matched = -1;
        for (let i = parseStack.length - 1; i >= 0; i--) {
          if (parseStack[i].path === s.path) {
            matched = i;
            break;
          }
        }
        const openParse = matched >= 0 ? parseStack.splice(matched, 1)[0] : null;
        const parseStart = openParse ? openParse.startMs : stepStartMs;
        const spanDur = Math.max(0.001, ts - parseStart);

        frames.push({
          id: openParse ? openParse.id : `parse-${row.id}-${index++}`,
          name: `Parse: ${fileName}`,
          type: 'parser',
          typeLabel: 'Parser',
          startMs: parseStart,
          durationMs: spanDur,
          depth: openParse ? openParse.depth : stackDepth,
          file: s.path || tmpl,
          line: 0,
          stackTrace: 'TextParser Engine',
          details: `Parse: ${parseStart.toFixed(3)} ms &ndash; ${ts.toFixed(3)} ms (${spanDur.toFixed(3)} ms)`,
          leftPercent: (parseStart / totalDur) * 100,
          widthPercent: Math.max((spanDur / totalDur) * 100, 0.4),
        });
      } else if (s.type === 'COMPILE_START') {
        compileStack.push({
          id: `compile-${row.id}-${index++}`,
          path: s.path,
          startMs: ts,
          depth: stackDepth,
        });
      } else if (s.type === 'COMPILE_END') {
        let matched = -1;
        for (let i = compileStack.length - 1; i >= 0; i--) {
          if (compileStack[i].path === s.path) {
            matched = i;
            break;
          }
        }
        const openCompile = matched >= 0 ? compileStack.splice(matched, 1)[0] : null;
        const compileStart = openCompile ? openCompile.startMs : stepStartMs;
        const spanDur = Math.max(0.001, ts - compileStart);

        frames.push({
          id: openCompile ? openCompile.id : `compile-${row.id}-${index++}`,
          name: `Compile: ${fileName}`,
          type: 'compiler',
          typeLabel: 'JIT Compiler',
          startMs: compileStart,
          durationMs: spanDur,
          depth: openCompile ? openCompile.depth : stackDepth,
          file: s.path || tmpl,
          line: 0,
          stackTrace: 'LLVM JIT Compiler',
          details: `JIT Compile: ${compileStart.toFixed(3)} ms &ndash; ${ts.toFixed(3)} ms (${spanDur.toFixed(3)} ms)`,
          leftPercent: (compileStart / totalDur) * 100,
          widthPercent: Math.max((spanDur / totalDur) * 100, 0.4),
        });
      } else if (s.type === 'ENTRY') {
        // Track open entry block
        entryStack.push({
          id: `entry-${row.id}-${index++}`,
          type: s.function ? 'component' : 'template',
          path: s.path,
          function: s.function,
          startMs: ts,
          depth: stackDepth,
          stackTrace: s.stackTrace,
        });
      } else if (s.type === 'EXIT') {
        // Close matching entry block and create its encompassing flame frame
        let matched = -1;
        for (let i = entryStack.length - 1; i >= 0; i--) {
          if (entryStack[i].path === s.path) {
            matched = i;
            break;
          }
        }
        const openEntry = matched >= 0 ? entryStack.splice(matched, 1)[0] : null;
        const entryStart = openEntry ? openEntry.startMs : stepStartMs;
        const spanDur = Math.max(0.001, ts - entryStart);
        const name = s.function && s.function.length > 0
          ? `${s.function}()`
          : fileName;

        frames.push({
          id: `span-${row.id}-${index++}`,
          name,
          type: openEntry ? openEntry.type as FlameEventType : 'template',
          typeLabel: s.function ? 'CFC Method' : 'Template',
          startMs: entryStart,
          durationMs: spanDur,
          depth: openEntry ? openEntry.depth : stackDepth,
          file: s.path || tmpl,
          line: s.line,
          stackTrace: s.stackTrace || (s.path ? `${s.path}:${s.line}` : ''),
          details: `Span: ${entryStart.toFixed(3)} ms &ndash; ${ts.toFixed(3)} ms (${spanDur.toFixed(3)} ms)`,
          leftPercent: (entryStart / totalDur) * 100,
          widthPercent: Math.max((spanDur / totalDur) * 100, 0.4),
        });
      } else if (s.type === 'ENGINE') {
        // Engine startup / lifecycle steps
        const name = s.function || s.path || 'Engine Step';
        frames.push({
          id: `engine-${row.id}-${index++}`,
          name,
          type: 'function',
          typeLabel: 'DB Engine',
          startMs: stepStartMs,
          durationMs: dur,
          depth: 1,
          file: '[ENGINE]',
          line: 0,
          stackTrace: 'Engine Internal Pipeline',
          details: `${name} (${stepStartMs.toFixed(3)} ms &ndash; ${stepEndMs.toFixed(3)} ms)`,
          leftPercent: (stepStartMs / totalDur) * 100,
          widthPercent: Math.max((dur / totalDur) * 100, 0.4),
        });
      } else {
        // Line execution, Query, Tag, Catch, etc.
        const type = this.mapStepType(s.type);
        const typeLabel = this.formatTypeLabel(s.type);
        const name = s.function && s.function.length > 0
          ? `${s.function}()`
          : (s.line > 0 ? `${fileName}:${s.line}` : s.type);

        frames.push({
          id: `step-${row.id}-${index++}`,
          name,
          type,
          typeLabel,
          startMs: stepStartMs,
          durationMs: dur,
          depth: stackDepth + 1,
          file: s.path || tmpl,
          line: s.line,
          stackTrace: s.stackTrace || (s.path ? `${s.path}:${s.line}` : ''),
          details: `Event: ${s.type}, Start: ${stepStartMs.toFixed(3)} ms, End: ${stepEndMs.toFixed(3)} ms (${dur.toFixed(3)} ms)`,
          leftPercent: (stepStartMs / totalDur) * 100,
          widthPercent: Math.max((dur / totalDur) * 100, 0.4),
        });
      }
    }

    // Close any remaining open entries
    while (entryStack.length > 0) {
      const open = entryStack.pop()!;
      const spanDur = Math.max(0.001, totalDur - open.startMs);
      const fileName = open.path ? open.path.split('/').pop() || open.path : tmpl;
      const name = open.function ? `${open.function}()` : fileName;

      frames.push({
        id: `open-${row.id}-${index++}`,
        name,
        type: open.type as FlameEventType,
        typeLabel: open.function ? 'CFC Method' : 'Template',
        startMs: open.startMs,
        durationMs: spanDur,
        depth: open.depth,
        file: open.path || tmpl,
        line: 0,
        stackTrace: open.stackTrace,
        details: `Span: ${open.startMs.toFixed(3)} ms &ndash; ${totalDur.toFixed(3)} ms (${spanDur.toFixed(3)} ms)`,
        leftPercent: (open.startMs / totalDur) * 100,
        widthPercent: Math.max((spanDur / totalDur) * 100, 0.4),
      });
    }

    return frames;
  }

  private mapStepType(type: string): FlameEventType {
    const t = (type || '').toUpperCase();
    if (t.includes('PARSE')) return 'parser';
    if (t.includes('COMPILE')) return 'compiler';
    if (t.includes('QUERY') || t.includes('DB_')) return 'query';
    if (t.includes('CFC') || t.includes('COMPONENT') || t.includes('METHOD')) return 'component';
    if (t.includes('CUSTOM_TAG') || t.includes('TAG')) return 'custom_tag';
    if (t.includes('INCLUDE')) return 'include';
    if (t.includes('ERROR') || t.includes('CATCH') || t.includes('EXCEPTION')) return 'error';
    if (t.includes('HTTP')) return 'http';
    if (t.includes('ENTRY') || t.includes('TEMPLATE') || t.includes('LINE')) return 'template';
    if (t.includes('ENGINE') || t.includes('FUNCTION')) return 'function';
    return 'template';
  }

  private formatTypeLabel(type: string): string {
    const t = (type || '').toUpperCase();
    if (t.includes('PARSE')) return 'Parser';
    if (t.includes('COMPILE')) return 'JIT Compiler';
    if (t.includes('QUERY')) return 'CFQUERY';
    if (t.includes('CFC')) return 'CFC Method';
    if (t.includes('CUSTOM_TAG')) return 'CFTAG';
    if (t.includes('INCLUDE')) return 'CFINCLUDE';
    if (t.includes('ERROR') || t.includes('CATCH')) return 'CFCATCH';
    if (t.includes('HTTP')) return 'CFHTTP';
    if (t.includes('ENGINE')) return 'DB Engine';
    if (t.includes('ENTRY')) return 'Template Entry';
    if (t.includes('LINE')) return 'Line Execution';
    return type || 'Step';
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
      case 'parser': return 'frame-parser';
      case 'compiler': return 'frame-compiler';
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
