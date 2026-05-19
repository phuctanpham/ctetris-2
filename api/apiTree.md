# API Directory Structure

```
api/
├── AGENTS.md                      # Agent definitions and configurations
├── README.md                      # API documentation and overview
├── package.json                   # Node.js project metadata and dependencies
├── tsconfig.json                  # TypeScript compiler configuration
├── wrangler.json                  # Cloudflare Workers configuration
├── worker-configuration.d.ts      # TypeScript type definitions for worker config
│
├── migrations/                    # Database migration scripts
│   └── sync-board.sql            # SQL migration for board synchronization
│
└── src/                           # Source code directory
    └── index.ts                   # Main entry point for the worker
```

## Directory Descriptions

- **AGENTS.md**: Contains agent definitions and metadata for API agents
- **README.md**: API documentation, setup instructions, and usage guide
- **package.json**: Project dependencies and npm scripts configuration
- **tsconfig.json**: TypeScript compiler settings for the project
- **wrangler.json**: Cloudflare Workers platform configuration
- **worker-configuration.d.ts**: TypeScript type definitions for worker configuration
- **migrations/**: Database migration scripts for schema and data updates
- **src/**: Source code directory containing the worker implementation
