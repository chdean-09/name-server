/*
  Warnings:

  - You are about to drop the column `identifier` on the `Device` table. All the data in the column will be lost.

*/
-- DropIndex
DROP INDEX "Device_identifier_key";

-- AlterTable
ALTER TABLE "Device" DROP COLUMN "identifier";
