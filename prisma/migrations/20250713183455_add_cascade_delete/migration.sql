-- DropForeignKey
ALTER TABLE "Schedule" DROP CONSTRAINT "Schedule_deviceId_fkey";

-- AlterTable
ALTER TABLE "Device" ALTER COLUMN "userEmail" DROP DEFAULT;

-- AddForeignKey
ALTER TABLE "Schedule" ADD CONSTRAINT "Schedule_deviceId_fkey" FOREIGN KEY ("deviceId") REFERENCES "Device"("id") ON DELETE CASCADE ON UPDATE CASCADE;
