/*
  Warnings:

  - You are about to drop the column `lockDay` on the `Schedule` table. All the data in the column will be lost.
  - You are about to drop the column `lockTime` on the `Schedule` table. All the data in the column will be lost.
  - You are about to drop the column `unlockDay` on the `Schedule` table. All the data in the column will be lost.
  - You are about to drop the column `unlockTime` on the `Schedule` table. All the data in the column will be lost.
  - Added the required column `time` to the `Schedule` table without a default value. This is not possible if the table is not empty.
  - Added the required column `type` to the `Schedule` table without a default value. This is not possible if the table is not empty.

*/
-- CreateEnum
CREATE TYPE "ScheduleType" AS ENUM ('LOCK', 'UNLOCK');

-- AlterTable
ALTER TABLE "Device" ADD COLUMN     "userEmail" TEXT NOT NULL DEFAULT 'test@example.com';

-- AlterTable
ALTER TABLE "Schedule" DROP COLUMN "lockDay",
DROP COLUMN "lockTime",
DROP COLUMN "unlockDay",
DROP COLUMN "unlockTime",
ADD COLUMN     "days" TEXT[],
ADD COLUMN     "isEnabled" BOOLEAN NOT NULL DEFAULT true,
ADD COLUMN     "time" TEXT NOT NULL,
ADD COLUMN     "type" "ScheduleType" NOT NULL;

-- CreateTable
CREATE TABLE "User" (
    "id" TEXT NOT NULL,
    "email" TEXT NOT NULL,

    CONSTRAINT "User_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE UNIQUE INDEX "User_email_key" ON "User"("email");

-- AddForeignKey
ALTER TABLE "Device" ADD CONSTRAINT "Device_userEmail_fkey" FOREIGN KEY ("userEmail") REFERENCES "User"("email") ON DELETE RESTRICT ON UPDATE CASCADE;
