CiviCRM Entity
==============

This Drupal 7 module integrates CiviCRM data into Drupal as fieldable entities using
[Entity API][1].

If CiviCRM triggers [`hook_civicrm_pre()`][2] on `$op = 'delete'` for an object, that
class of objects can be entities in Drupal 7.

If your CiviCRM database is separate from your Drupal database, you must configure the
Drupal site's `settings.php` file with [settings that enable Drupal to access CiviCRM's
tables][4].

[1]: http://drupal.org/project/entity
[2]: http://wiki.civicrm.org/confluence/display/CRMDOC41/CiviCRM+hook+specification
[3]: http://drupal.org/project/calendar
[4]: http://wiki.civicrm.org/confluence/display/CRMDOC40/Views3+Integration

Real life
---------

This module was built to allow CiviEvent records to have a taxonomy term field. The
taxonomy term field was used in a [Calendar][3] module view of CiviEvent rows.

Extensive rewriting of the Views calendar row handler plugin for CiviEvent records was
necessary because the handler included with CiviCRM is stripped of the ability to render
a calendar legend based on taxonomy term reference field values.

Some work was done to attach fields to CiviCRM groups, but it may be unfinished.
