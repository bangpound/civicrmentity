<?php

class CiviCRMEntity extends Entity {
}

class CiviCRMEvent extends CiviCRMEntity {
  protected function defaultUri() {
    return array(
      'path' => 'civicrm/event/info',
      'options' => array(
        'query' => array(
          'reset' => 1,
          'id' => $this->identifier(),
        ),
      ),
    );
  }
}
