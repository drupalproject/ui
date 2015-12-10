<?php

/**
 * Implements hook_theme
 *
 * @return array
 */
function ui_theme() {
  return array(
    'ui' => array(
      'arguments' => array('template' => NULL, 'data' => NULL),
      'template' => 'ui',
    ),
  );
}

/**
 * Implements hook_tag_info
 *
 * @return array
 */
function ui_tag_info() {
  return array(
    'pc' => array(
      'theme' => 'garland'
    ),

    'm' => array(
      'theme' => 'zmui',
    )
  );
}

/**
 * Implements hook_preprocess_ui
 *
 * @param $variables
 */
function ui_preprocess_ui(&$variables) {
  $tag = ui_select();

  if ($tag) {
    $variables['scripts'] = ui_get_js($tag);
    $variables['styles'] = ui_get_css($tag);
  }



}
