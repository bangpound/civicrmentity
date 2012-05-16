<?php

class CiviCRMEntityController extends EntityAPIController implements EntityAPIControllerInterface {

  /**
   * Static cache of entities.
   *
   * @var array
   */
  protected $entityCache;

  /**
   * Entity type for this controller instance.
   *
   * @var string
   */
  protected $entityType;

  /**
   * Array of information about the entity.
   *
   * @var array
   *
   * @see entity_get_info()
   */
  protected $entityInfo;

  /**
   * Additional arguments to pass to hook_TYPE_load().
   *
   * Set before calling DrupalDefaultEntityController::attachLoad().
   *
   * @var array
   */
  protected $hookLoadArguments;

  /**
   * Name of the entity's ID field in the entity database table.
   *
   * @var string
   */
  protected $idKey;

  /**
   * Name of entity's revision database table field, if it supports revisions.
   *
   * Has the value FALSE if this entity does not use revisions.
   *
   * @var string
   */
  protected $revisionKey;

  /**
   * The table that stores revisions, if the entity supports revisions.
   *
   * @var string
   */
  protected $revisionTable;

  /**
   * Whether this entity type should use the static cache.
   *
   * Set by entity info.
   *
   * @var boolean
   */
  protected $cache;

  /**
   * Constructor: sets basic variables.
   */
  public function __construct($entityType) {
    $this->entityType = $entityType;
    $this->entityInfo = entity_get_info($entityType);
    $this->entityCache = array();
    $this->hookLoadArguments = array();
    $this->idKey = $this->entityInfo['entity keys']['id'];

    // Check if the entity type supports revisions.
    if (!empty($this->entityInfo['entity keys']['revision'])) {
      $this->revisionKey = $this->entityInfo['entity keys']['revision'];
      $this->revisionTable = $this->entityInfo['revision table'];
    }
    else {
      $this->revisionKey = FALSE;
    }

    // Check if the entity type supports static caching of loaded entities.
    $this->cache = !empty($this->entityInfo['static cache']);
  }

  /**
   * Implements DrupalEntityControllerInterface::resetCache().
   */
  public function resetCache(array $ids = NULL) {
    if (isset($ids)) {
      foreach ($ids as $id) {
        unset($this->entityCache[$id]);
      }
    }
    else {
      $this->entityCache = array();
    }
  }

  /**
   * Implements DrupalEntityControllerInterface::load().
   */
  public function load($ids = array(), $conditions = array()) {
    $entities = array();

    // Revisions are not statically cached, and require a different query to
    // other conditions, so separate the revision id into its own variable.
    if ($this->revisionKey && isset($conditions[$this->revisionKey])) {
      $revision_id = $conditions[$this->revisionKey];
      unset($conditions[$this->revisionKey]);
    }
    else {
      $revision_id = FALSE;
    }

    // Create a new variable which is either a prepared version of the $ids
    // array for later comparison with the entity cache, or FALSE if no $ids
    // were passed. The $ids array is reduced as items are loaded from cache,
    // and we need to know if it's empty for this reason to avoid querying the
    // database when all requested entities are loaded from cache.
    $passed_ids = !empty($ids) ? array_flip($ids) : FALSE;
    // Try to load entities from the static cache, if the entity type supports
    // static caching.
    if ($this->cache && !$revision_id) {
      $entities += $this->cacheGet($ids, $conditions);
      // If any entities were loaded, remove them from the ids still to load.
      if ($passed_ids) {
        $ids = array_keys(array_diff_key($passed_ids, $entities));
      }
    }

    // Load any remaining entities from the database. This is the case if $ids
    // is set to FALSE (so we load all entities), if there are any ids left to
    // load, if loading a revision, or if $conditions was passed without $ids.
    if ($ids === FALSE || $ids || $revision_id || ($conditions && !$passed_ids)) {
      if (!civicrm_initialize()) {
        return;
      }
      require_once 'api/api.php';

      $queried_entities = array();
      if ($ids !== FALSE) {
        foreach ($ids as $id) {
          $params = array(
            'id' => $id,
            'version' => 3,
          );
          $result = civicrm_api($this->entityInfo['civicrm']['entity'], 'Get', $params);
          if (!empty($this->entityInfo['entity class'])) {
            $queried_entities[$id] =  new $this->entityInfo['entity class']($result['values'][$id], $this->entityType);
          }
          else {
            $queried_entities[$id] = (object) $result['values'][$id];
          }
        }
      }
      else {
        $params = array(
          'version' => 3,
        );
        $result = civicrm_api($this->entityInfo['civicrm']['entity'], 'Get', $params);
        foreach ($result['values'] as $id => $values) {
          if (!empty($this->entityInfo['entity class'])) {
            $queried_entities[$id] =  new $this->entityInfo['entity class']($values, $this->entityType);
          }
          else {
            $queried_entities[$id] = (object) $values;
          }
        }
      }
    }

    // Pass all entities loaded from the database through $this->attachLoad(),
    // which attaches fields (if supported by the entity type) and calls the
    // entity type specific load callback, for example hook_node_load().
    if (!empty($queried_entities)) {
      $this->attachLoad($queried_entities, $revision_id);
      $entities += $queried_entities;
    }

    if ($this->cache) {
      // Add entities to the cache if we are not loading a revision.
      if (!empty($queried_entities) && !$revision_id) {
        $this->cacheSet($queried_entities);
      }
    }

    // Ensure that the returned array is ordered the same as the original
    // $ids array if this was passed in and remove any invalid ids.
    if ($passed_ids) {
      // Remove any invalid ids from the array.
      $passed_ids = array_intersect_key($passed_ids, $entities);
      foreach ($entities as $entity) {
        $passed_ids[$entity->{$this->idKey}] = $entity;
      }
      $entities = $passed_ids;
    }

    return $entities;
  }

  /**
   * Attaches data to entities upon loading.
   * This will attach fields, if the entity is fieldable. It calls
   * hook_entity_load() for modules which need to add data to all entities.
   * It also calls hook_TYPE_load() on the loaded entities. For example
   * hook_node_load() or hook_user_load(). If your hook_TYPE_load()
   * expects special parameters apart from the queried entities, you can set
   * $this->hookLoadArguments prior to calling the method.
   * See NodeController::attachLoad() for an example.
   *
   * @param $queried_entities
   *   Associative array of query results, keyed on the entity ID.
   * @param $revision_id
   *   ID of the revision that was loaded, or FALSE if the most current revision
   *   was loaded.
   */
  protected function attachLoad(&$queried_entities, $revision_id = FALSE) {
    // Attach fields.
    if ($this->entityInfo['fieldable']) {
      if ($revision_id) {
        field_attach_load_revision($this->entityType, $queried_entities);
      }
      else {
        field_attach_load($this->entityType, $queried_entities);
      }
    }

    // Call hook_entity_load().
    foreach (module_implements('entity_load') as $module) {
      $function = $module . '_entity_load';
      $function($queried_entities, $this->entityType);
    }
    // Call hook_TYPE_load(). The first argument for hook_TYPE_load() are
    // always the queried entities, followed by additional arguments set in
    // $this->hookLoadArguments.
    $args = array_merge(array($queried_entities), $this->hookLoadArguments);
    foreach (module_implements($this->entityInfo['load hook']) as $module) {
      call_user_func_array($module . '_' . $this->entityInfo['load hook'], $args);
    }
  }

  /**
   * Gets entities from the static cache.
   *
   * @param $ids
   *   If not empty, return entities that match these IDs.
   * @param $conditions
   *   If set, return entities that match all of these conditions.
   *
   * @return
   *   Array of entities from the entity cache.
   */
  protected function cacheGet($ids, $conditions = array()) {
    $entities = array();
    // Load any available entities from the internal cache.
    if (!empty($this->entityCache)) {
      if ($ids) {
        $entities += array_intersect_key($this->entityCache, array_flip($ids));
      }
      // If loading entities only by conditions, fetch all available entities
      // from the cache. Entities which don't match are removed later.
      elseif ($conditions) {
        $entities = $this->entityCache;
      }
    }

    // Exclude any entities loaded from cache if they don't match $conditions.
    // This ensures the same behavior whether loading from memory or database.
    if ($conditions) {
      foreach ($entities as $entity) {
        $entity_values = (array) $entity;
        if (array_diff_assoc($conditions, $entity_values)) {
          unset($entities[$entity->{$this->idKey}]);
        }
      }
    }
    return $entities;
  }

  /**
   * Stores entities in the static entity cache.
   *
   * @param $entities
   *   Entities to store in the cache.
   */
  protected function cacheSet($entities) {
    $this->entityCache += $entities;
  }

  /**
   * Implements EntityAPIControllerInterface.
   *
   * @param $transaction
   *   Optionally a DatabaseTransaction object to use. Allows overrides to pass
   *   in their transaction object.
   */
  public function delete($ids, DatabaseTransaction $transaction = NULL) {
    $entities = $ids ? $this->load($ids) : FALSE;
    if (!$entities) {
      // Do nothing, in case invalid or no ids have been passed.
      return;
    }
    // This transaction causes troubles on MySQL, see
    // http://drupal.org/node/1007830. So we deactivate this by default until
    // is shipped in a point release.
    // $transaction = isset($transaction) ? $transaction : db_transaction();

    try {
      $ids = array_keys($entities);

      //db_delete($this->entityInfo['base table'])
      //  ->condition($this->idKey, $ids, 'IN')
      //  ->execute();
      // Reset the cache as soon as the changes have been applied.
      $this->resetCache($ids);

      foreach ($entities as $id => $entity) {
        $this->invoke('delete', $entity);
      }
      // Ignore slave server temporarily.
      // db_ignore_slave();
    }
    catch (Exception $e) {
      if (isset($transaction)) {
        //$transaction->rollback();
      }
      watchdog_exception($this->entityType, $e);
      throw $e;
    }
  }

  /**
   * Implements EntityAPIControllerInterface.
   *
   * @param $transaction
   *   Optionally a DatabaseTransaction object to use. Allows overrides to pass
   *   in their transaction object.
   */
  public function save($entity, DatabaseTransaction $transaction = NULL) {
    //$transaction = isset($transaction) ? $transaction : db_transaction();
    try {
      // Load the stored entity, if any.
      if (!empty($entity->{$this->idKey}) && !isset($entity->original)) {
        // In order to properly work in case of name changes, load the original
        // entity using the id key if it is available.
        $entity->original = entity_load_unchanged($this->entityType, $entity->{$this->idKey});
      }

      $this->invoke('presave', $entity);

      if (!empty($entity->{$this->idKey}) && empty($entity->is_new)) {
        //$return = drupal_write_record($this->entityInfo['base table'], $entity, $this->idKey);
        $this->resetCache(array($entity->{$this->idKey}));
        $this->invoke('update', $entity);
      }
      else {
        //$return = drupal_write_record($this->entityInfo['base table'], $entity);
        $this->invoke('insert', $entity);
      }
      // Ignore slave server temporarily.
      //db_ignore_slave();
      unset($entity->is_new);
      unset($entity->original);

      return $return;
    }
    catch (Exception $e) {
      //$transaction->rollback();
      watchdog_exception($this->entityType, $e);
      throw $e;
    }
  }

  /**
   * Builds and executes the query for loading.
   *
   * @return The results in a Traversable object.
   */
  public function query($ids, $conditions, $revision_id = FALSE) {
    // Build the query.
    $result = entity_load($this->entityType, $ids);
    return $result;
  }
}
