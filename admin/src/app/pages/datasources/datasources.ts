import { Component, inject, OnInit, signal } from '@angular/core';
import { FormsModule } from '@angular/forms';

import { AdminApiService } from '../../services/admin-api.service';

interface DataSourceRow {
  name: string;
  backend: string;
  backendLabel: string;
  database: string;
  host: string;
  port: string;
  username: string;
  verified: boolean | null;
}

interface DatasourceForm {
  name: string;
  backend: string;
  host: string;
  port: string;
  database: string;
  username: string;
  password: string;
}

// Display casing for the backend names (the stored value stays lowercase).
const BACKEND_LABELS: Record<string, string> = {
  sqlite: 'SQLite',
  mysql: 'MySQL',
  postgres: 'PostgreSQL',
  postgresql: 'PostgreSQL',
  pg: 'PostgreSQL',
};

function emptyForm(): DatasourceForm {
  return { name: '', backend: 'sqlite', host: '', port: '', database: '', username: '', password: '' };
}

@Component({
  selector: 'app-datasources',
  imports: [FormsModule],
  templateUrl: './datasources.html',
  styleUrl: './datasources.css',
})
export class Datasources implements OnInit {
  private api = inject(AdminApiService);

  // Signals so the zoneless app re-renders when async API responses land.
  protected readonly sources = signal<DataSourceRow[]>([]);
  protected readonly loaded = signal(false);
  protected readonly busy = signal(false);
  protected readonly message = signal('');
  protected readonly form = signal<DatasourceForm>(emptyForm());
  // Non-empty while the form is editing an existing datasource (its name).
  protected readonly editingName = signal('');

  ngOnInit() {
    this.refresh();
  }

  // Immutable form-field update so the signal notifies the zoneless app (a
  // direct [(ngModel)] mutation would not re-render the conditional fields).
  protected setField(key: keyof DatasourceForm, value: string) {
    this.form.update((f) => ({ ...f, [key]: value }));
  }

  protected refresh() {
    this.api.getDatasources().subscribe({
      next: (ds) => {
        this.sources.set(
          Object.entries(ds).map(([name, d]) => ({
            name,
            backend: d.backend,
            backendLabel: BACKEND_LABELS[d.backend] ?? d.backend,
            // SQLite has no separate database setting: the file is DNS_<name>.sqlite.
            database: d.backend === 'sqlite' ? `DNS_${name}.sqlite` : d.database,
            host: d.host || '-',
            port: d.port ? String(d.port) : '-',
            username: d.username,
            verified: null,
          })),
        );
        this.loaded.set(true);
      },
      error: () => {
        this.loaded.set(true);
      },
    });
  }

  // Loads an existing datasource into the form for editing. The password is
  // left blank (the server keeps the existing one when no new value is sent).
  protected edit(name: string) {
    const row = this.sources().find((s) => s.name === name);
    if (!row) return;
    this.form.set({
      name: row.name,
      backend: row.backend,
      host: row.host === '-' ? '' : row.host,
      port: row.port === '-' ? '' : row.port,
      database: row.backend === 'sqlite' ? '' : row.database,
      username: row.username,
      password: '',
    });
    this.editingName.set(name);
    this.message.set('');
  }

  protected cancelEdit() {
    this.editingName.set('');
    this.form.set(emptyForm());
  }

  protected verify(name: string) {
    this.message.set('');
    this.api.verifyDatasource(name).subscribe({
      next: (r) => {
        // Replace the array so the signal notifies the zoneless app.
        this.sources.update((rows) =>
          rows.map((row) => (row.name === name ? { ...row, verified: r.verified } : row)),
        );
        this.message.set(r.verified ? `Datasource '${name}' verified.` : `Datasource '${name}': ${r.error ?? 'unreachable'}`);
      },
      error: (err) => {
        this.sources.update((rows) =>
          rows.map((row) => (row.name === name ? { ...row, verified: false } : row)),
        );
        this.message.set(err.error?.error ?? 'Verification failed.');
      },
    });
  }

  protected verifyAll() {
    for (const s of this.sources()) {
      this.verify(s.name);
    }
  }

  protected remove(name: string) {
    this.busy.set(true);
    this.message.set('');
    this.api.deleteDatasource(name).subscribe({
      next: () => {
        this.busy.set(false);
        if (this.editingName() === name) this.cancelEdit();
        this.refresh();
      },
      error: (err) => {
        this.busy.set(false);
        this.message.set(err.error?.error ?? 'Delete failed.');
      },
    });
  }

  protected save() {
    const name = this.form().name.trim();
    if (!name) {
      this.message.set('A data source name is required.');
      return;
    }
    this.busy.set(true);
    this.message.set('');
    const f = this.form();
    // SQLite needs only the name; the server fields apply to mysql/postgres.
    const payload: { name: string; backend: string; host?: string; port?: number; database?: string; username?: string; password?: string } = {
      name,
      backend: f.backend,
    };
    if (f.backend !== 'sqlite') {
      if (f.host) payload.host = f.host;
      if (f.port) payload.port = Number(f.port);
      if (f.database) payload.database = f.database;
      if (f.username) payload.username = f.username;
      if (f.password) payload.password = f.password;
    }
    this.api
      .upsertDatasource(payload)
      .subscribe({
        next: () => {
          this.busy.set(false);
          this.message.set(`Datasource '${name}' saved.`);
          this.cancelEdit();
          this.refresh();
        },
        error: (err) => {
          this.busy.set(false);
          this.message.set(err.error?.error ?? 'Save failed.');
        },
      });
  }
}
