#!/bin/bash

# Development setup script

echo "🚀 Setting up ESP32 Door Lock System..."

# Check if Node.js is installed
if ! command -v node &> /dev/null; then
    echo "❌ Node.js is not installed. Please install Node.js first."
    exit 1
fi

# Check if npm is installed
if ! command -v npm &> /dev/null; then
    echo "❌ npm is not installed. Please install npm first."
    exit 1
fi

# Install dependencies
echo "📦 Installing dependencies..."
npm install

# Check if .env file exists
if [ ! -f .env ]; then
    echo "📝 Creating .env file from template..."
    cp env.example .env
    echo "⚠️  Please update the DATABASE_URL in .env file"
fi

# Generate Prisma client
echo "🗄️  Generating Prisma client..."
npx prisma generate

# Check if database migration is needed
echo "🔄 Checking database status..."
if npx prisma migrate status 2>/dev/null | grep -q "Database is up to date"; then
    echo "✅ Database is up to date"
else
    echo "🔄 Running database migrations..."
    npx prisma migrate dev --name init
fi

echo "✅ Setup complete!"
echo ""
echo "🎯 Next steps:"
echo "1. Update the DATABASE_URL in .env file"
echo "2. Update the serverURL in door-lock.ino"
echo "3. Run 'npm run start:dev' to start the server"
echo "4. Flash the ESP32 with the updated code"
echo ""
echo "📚 Check SYSTEM_README.md for detailed instructions"
