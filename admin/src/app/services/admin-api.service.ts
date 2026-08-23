import { Injectable, inject } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';

export interface AdminSettings {
  enableWhitespaceManagement: boolean;
  defaultOutputCharset: string;
  defaultInputCharset: string;
  charsetDetectionMinConfidence: number;
  scopeDbPath: string;
  cacheDbPath: string;
  dsnDbDir: string;
  defaultApplicationTimeoutSeconds: number;
  defaultSessionTimeoutSeconds: number;
  enableQueryLogging: boolean;
  debugEnabled: boolean;
  compileExtForInclude: string;
}

export interface Datasource {
  backend: string;
  host: string;
  port: number;
  database: string;
  username: string;
  password: string;
}

export interface AdminConfig {
  settings: AdminSettings;
  datasources: Record<string, Datasource>;
}

export interface RecentRequest {
  time: number;
  template: string;
  method: string;
  status: number;
  durationMs: number;
}

export interface ServerInfo {
  state: string;
  version: string;
  uptimeSeconds: number;
  requestsServed: number;
  avgResponseMs: number;
  recentRequests: RecentRequest[];
}

export interface DatasourceTestResult {
  verified: boolean;
  error?: string;
}

export interface CacheEntry {
  region: string;
  id: string;
  hits: number;
  size: number;
  createdMs: number;
  lastAccessMs: number;
  lastUpdateMs: number;
  expiresMs: number; // 0 = eternal
}

export interface CacheInfo {
  entries: CacheEntry[];
  totalEntries: number;
  totalHits: number;
  totalSize: number;
  compiledTemplates: number;   // JIT-compiled .cfm held by this worker
  compiledComponents: number;  // JIT-compiled .cfc held by this worker
}

export interface ApiError {
  ok?: boolean;
  error?: string;
}

@Injectable({ providedIn: 'root' })
export class AdminApiService {
  private http = inject(HttpClient);
  private base = '/admin/api';

  getConfig(): Observable<AdminConfig> {
    return this.http.get<AdminConfig>(`${this.base}/config.cfm`);
  }

  updateConfig(payload: { settings?: Partial<AdminSettings>; datasources?: Record<string, Partial<Datasource>> }): Observable<AdminConfig> {
    return this.http.post<AdminConfig>(`${this.base}/config.cfm`, payload);
  }

  resetConfig(): Observable<AdminConfig> {
    return this.http.post<AdminConfig>(`${this.base}/config.cfm`, { action: 'reset' });
  }

  getDatasources(): Observable<Record<string, Datasource>> {
    return this.http.get<Record<string, Datasource>>(`${this.base}/datasources.cfm`);
  }

  upsertDatasource(ds: { name: string; backend?: string; host?: string; port?: number; database?: string; username?: string; password?: string }): Observable<AdminConfig> {
    return this.http.post<AdminConfig>(`${this.base}/datasources.cfm`, ds);
  }

  deleteDatasource(name: string): Observable<AdminConfig> {
    return this.http.post<AdminConfig>(`${this.base}/datasources.cfm`, { name, action: 'delete' });
  }

  verifyDatasource(name: string): Observable<DatasourceTestResult> {
    return this.http.post<DatasourceTestResult>(`${this.base}/datasources.cfm`, { name, action: 'verify' });
  }

  getServerInfo(excludeAdmin = true): Observable<ServerInfo> {
    return this.http.get<ServerInfo>(`${this.base}/serverinfo.cfm`, {
      params: { excludeAdmin: excludeAdmin ? 'true' : 'false' },
    });
  }

  getCacheInfo(): Observable<CacheInfo> {
    return this.http.get<CacheInfo>(`${this.base}/cache.cfm`);
  }

  clearCache(): Observable<{ ok: boolean }> {
    return this.http.post<{ ok: boolean }>(`${this.base}/cache.cfm`, { action: 'clear' });
  }

  evictCacheEntry(region: string, id: string): Observable<{ ok: boolean; error?: string }> {
    return this.http.post<{ ok: boolean; error?: string }>(`${this.base}/cache.cfm`, { action: 'evict', region, id });
  }
}
