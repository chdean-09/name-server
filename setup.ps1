# Development setup script for Windows

Write-Host "🚀 Setting up ESP32 Door Lock System..." -ForegroundColor Green

# Check if Node.js is installed
if (-not (Get-Command node -ErrorAction SilentlyContinue)) {
    Write-Host "❌ Node.js is not installed. Please install Node.js first." -ForegroundColor Red
    exit 1
}

# Check if npm is installed
if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
    Write-Host "❌ npm is not installed. Please install npm first." -ForegroundColor Red
    exit 1
}

# Install dependencies
Write-Host "📦 Installing dependencies..." -ForegroundColor Yellow
npm install

# Check if .env file exists
if (-not (Test-Path .env)) {
    Write-Host "📝 Creating .env file from template..." -ForegroundColor Yellow
    Copy-Item env.example .env
    Write-Host "⚠️  Please update the DATABASE_URL in .env file" -ForegroundColor Yellow
}

# Generate Prisma client
Write-Host "🗄️  Generating Prisma client..." -ForegroundColor Yellow
npx prisma generate

# Check if database migration is needed
Write-Host "🔄 Checking database status..." -ForegroundColor Yellow
try {
    $migrateStatus = npx prisma migrate status 2>$null
    if ($migrateStatus -match "Database is up to date") {
        Write-Host "✅ Database is up to date" -ForegroundColor Green
    } else {
        Write-Host "🔄 Running database migrations..." -ForegroundColor Yellow
        npx prisma migrate dev --name init
    }
} catch {
    Write-Host "🔄 Running database migrations..." -ForegroundColor Yellow
    npx prisma migrate dev --name init
}

Write-Host "✅ Setup complete!" -ForegroundColor Green
Write-Host ""
Write-Host "🎯 Next steps:" -ForegroundColor Cyan
Write-Host "1. Update the DATABASE_URL in .env file"
Write-Host "2. Update the serverURL in door-lock.ino"
Write-Host "3. Run 'npm run start:dev' to start the server"
Write-Host "4. Flash the ESP32 with the updated code"
Write-Host ""
Write-Host "📚 Check SYSTEM_README.md for detailed instructions" -ForegroundColor Cyan
