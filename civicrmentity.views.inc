<?php

function civicrmentity_views_data_alter(&$data) {
  $data['civicrm_contact']['table']['entity type'] = 'civicrm_contact';
  $data['civicrm_group']['table']['entity type'] = 'civicrm_group';
}
