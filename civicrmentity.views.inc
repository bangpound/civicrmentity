<?php

function civicrmentity_views_data_alter(&$data) {
  $data['civicrm_contact']['table']['entity type'] = 'civicrm_contact';
  $data['civicrm_group']['table']['entity type'] = 'civicrm_group';
}

/**
 * Implements hook_views_plugins
 */
function civicrmentity_views_plugins_alter(&$plugins) {
  foreach ($plugins['row'] as &$plugin) {
    if ($plugin['handler'] == 'calendar_plugin_row_civicrm') {
      $plugin['path'] = drupal_get_path('module', 'civicrmentity');
      $plugin['module'] = 'civicrmentity';
      $plugin['handler'] = 'civicrmentity_calendar_plugin_row';
      $plugin['file'] = 'civicrmentity_calendar_plugin_row.inc';
      $plugin['theme'] = $plugins['row']['fields']['theme file'];
      $plugin['theme file'] = $plugins['row']['fields']['theme file'];
      $plugin['theme path'] = $plugins['row']['fields']['theme path'];
    }
  }
}
