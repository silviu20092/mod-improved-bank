DROP TABLE IF EXISTS `mod_improved_bank`;
CREATE TABLE `mod_improved_bank` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `owner_guid` int unsigned NOT NULL,
  `owner_account` int unsigned NOT NULL,
  `item_entry` int unsigned NOT NULL,
  `item_count` int unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Mod improved bank table';