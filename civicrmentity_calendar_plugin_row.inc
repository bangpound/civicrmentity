<?php
/**
 * @file
 * Contains the Calendar row style plugin.
 *
 * This plugin takes the view results, finds the date value for each,
 * then compares that date to the date range for the current view.
 * Items that started before or ended after the current date range
 * are shortened to the current range. Items that extend over more
 * than one day are cloned to create a calendar item for each day.
 * The resulting array of results (which may have a different number
 * of items than the original view result) are then passed back
 * to the style plugin so they can be displayed in a calendar.
 *
 */

/**
 * Plugin which creates a view on the resulting object
 * and formats it as a Calendar node.
 */
class civicrmentity_calendar_plugin_row extends calendar_plugin_row {

  function init(&$view, &$display, $options = NULL) {
    if (!civicrm_initialize()) {
      return;
    }
    parent::init($view, $display, $options);
  }

  function option_definition() {
    $options = parent::option_definition();
    unset($options['colors']['contains']['taxonomy_field']);
    unset($options['colors']['contains']['calendar_colors_vocabulary']);
    unset($options['colors']['contains']['calendar_colors_taxonomy']);
    unset($options['colors']['contains']['calendar_colors_group']);
    return $options;
  }

  /**
   * Provide a form for setting options.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['colors']['legend'] = array(
      '#type' => 'value',
    );
  }

  function pre_render($values) {
    parent::pre_render($values);
    $this->date_fields['start_date']['tz_handling'] = 'none';
  }
}
