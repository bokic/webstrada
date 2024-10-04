import { ApplicationConfig, provideBrowserGlobalErrorListeners, provideZonelessChangeDetection } from '@angular/core';
import { provideHttpClient } from '@angular/common/http';
import { provideRouter } from '@angular/router';

import { routes } from './app.routes';

export const appConfig: ApplicationConfig = {
  providers: [
    provideBrowserGlobalErrorListeners(),
    // The app is zoneless (no zone.js bundled); without this explicit provider
    // async HttpClient responses never trigger change detection, so page data
    // (e.g. the dashboard stats) would never render.
    provideZonelessChangeDetection(),
    provideHttpClient(),
    provideRouter(routes)
  ]
};
