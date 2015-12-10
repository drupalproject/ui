<?php

/**
 * Base on core drupal_add_css, but support multiple tags
 *
 * @param $tag
 * @param null $path
 * @param string $type
 * @param string $media
 * @param bool|TRUE $preprocess
 * @return mixed
 */
function ui_add_css($tag, $path = NULL, $type = 'module', $media = 'all', $preprocess = TRUE) {
  static $css = array();
  global $language;

  if (!isset($css[$tag])) {
    $css[$tag] = array();
  }

  // Create an array of CSS files for each media type first, since each type needs to be served
  // to the browser differently.
  if (isset($path)) {
    // This check is necessary to ensure proper cascading of styles and is faster than an asort().
    if (!isset($css[$tag][$media])) {
      $css[$tag][$media] = array('module' => array(), 'theme' => array());
    }
    $css[$tag][$media][$type][$path] = $preprocess;

    // If the current language is RTL, add the CSS file with RTL overrides.
    if ($language->direction == LANGUAGE_RTL) {
      $rtl_path = str_replace('.css', '-rtl.css', $path);
      if (file_exists($rtl_path)) {
        $css[$tag][$media][$type][$rtl_path] = $preprocess;
      }
    }
  }

  return $css[$tag];
}

/**
 * Base on core drupal_get_css, but support multiple tags
 *
 * @param $tag
 * @param null $css
 * @return string
 */
function ui_get_css($tag, $css = NULL) {
  $output = '';
  if (!isset($css)) {
    $css = ui_add_css($tag);
  }
  $no_module_preprocess = '';
  $no_theme_preprocess = '';

  $preprocess_css = (variable_get('preprocess_css', FALSE) && (!defined('MAINTENANCE_MODE') || MAINTENANCE_MODE != 'update'));
  $directory = file_directory_path();
  $is_writable = is_dir($directory) && is_writable($directory) && (variable_get('file_downloads', FILE_DOWNLOADS_PUBLIC) == FILE_DOWNLOADS_PUBLIC);

  // A dummy query-string is added to filenames, to gain control over
  // browser-caching. The string changes on every update or full cache
  // flush, forcing browsers to load a new copy of the files, as the
  // URL changed.
  $query_string = '?'. variable_get('css_js_query_string', '0');

  foreach ($css as $media => $types) {
    // If CSS preprocessing is off, we still need to output the styles.
    // Additionally, go through any remaining styles if CSS preprocessing is on and output the non-cached ones.
    foreach ($types as $type => $files) {
      if ($type == 'module') {
        // Setup theme overrides for module styles.
        $theme_styles = array();
        foreach (array_keys($css[$media]['theme']) as $theme_style) {
          $theme_styles[] = basename($theme_style);
        }
      }
      foreach ($types[$type] as $file => $preprocess) {
        // If the theme supplies its own style using the name of the module style, skip its inclusion.
        // This includes any RTL styles associated with its main LTR counterpart.
        if ($type == 'module' && in_array(str_replace('-rtl.css', '.css', basename($file)), $theme_styles)) {
          // Unset the file to prevent its inclusion when CSS aggregation is enabled.
          unset($types[$type][$file]);
          continue;
        }
        // Only include the stylesheet if it exists.
        if (file_exists($file)) {
          if (!$preprocess || !($is_writable && $preprocess_css)) {
            // If a CSS file is not to be preprocessed and it's a module CSS file, it needs to *always* appear at the *top*,
            // regardless of whether preprocessing is on or off.
            if (!$preprocess && $type == 'module') {
              $no_module_preprocess .= '<link type="text/css" rel="stylesheet" media="'. $media .'" href="'. base_path() . $file . $query_string .'" />'."\n";
            }
            // If a CSS file is not to be preprocessed and it's a theme CSS file, it needs to *always* appear at the *bottom*,
            // regardless of whether preprocessing is on or off.
            else if (!$preprocess && $type == 'theme') {
              $no_theme_preprocess .= '<link type="text/css" rel="stylesheet" media="'. $media .'" href="'. base_path() . $file . $query_string .'" />'."\n";
            }
            else {
              $output .= '<link type="text/css" rel="stylesheet" media="'. $media .'" href="'. base_path() . $file . $query_string .'" />'."\n";
            }
          }
        }
      }
    }

    if ($is_writable && $preprocess_css) {
      // Prefix filename to prevent blocking by firewalls which reject files
      // starting with "ad*".
      //$filename = 'css_'. md5(serialize($types) . $query_string) .'.css';

      //@patch for css/js
      //Robbin changed core to keep the css/js filename same anywhere
      //#9346
      //$filename = 'css_'. md5(serialize($types)) .'.css';

      // #14216
      // Change md5 logic to make it sensitive for file change
      // It's better for 7niu CDN
      $filename = 'css_'. ui_files_md5('css', $types) .'.css';

      $preprocess_file = drupal_build_css_cache($types, $filename);

      //SHOULD get querystring again
      $query_string = '?'. variable_get('css_js_query_string', '0');
      $output .= '<link type="text/css" rel="stylesheet" media="'. $media .'" href="'. file_create_url($preprocess_file) . $query_string .'" />'."\n";
    }
  }

  return $no_module_preprocess . $output . $no_theme_preprocess;
}

/**
 * Add theme css resource for specific tag
 *
 * @param $tag
 */
function ui_load_theme_css($tag) {
  global $theme;
  $themes = list_themes();

  $base_theme = array();
  $ancestor = $theme;
  while ($ancestor && isset($themes[$ancestor]->base_theme)) {
    $base_theme[] = $new_base_theme = $themes[$themes[$ancestor]->base_theme];
    $ancestor = $themes[$ancestor]->base_theme;
  }

  $base_theme = array_reverse($base_theme);

  $theme_info = $themes[$theme];

  // Prepare stylesheets from this theme as well as all ancestor themes.
  // We work it this way so that we can have child themes override parent
  // theme stylesheets easily.
  $final_stylesheets = array();

  // Grab stylesheets from base theme
  foreach ($base_theme as $base) {
    if (!empty($base->stylesheets)) {
      foreach ($base->stylesheets as $media => $stylesheets) {
        foreach ($stylesheets as $name => $stylesheet) {
          $final_stylesheets[$media][$name] = $stylesheet;
        }
      }
    }
  }

  // Add stylesheets used by this theme.
  if (!empty($theme_info->stylesheets)) {
    foreach ($theme_info->stylesheets as $media => $stylesheets) {
      foreach ($stylesheets as $name => $stylesheet) {
        $final_stylesheets[$media][$name] = $stylesheet;
      }
    }
  }

  // And now add the stylesheets properly
  foreach ($final_stylesheets as $media => $stylesheets) {
    foreach ($stylesheets as $stylesheet) {
      ui_add_css($tag, $stylesheet, 'theme', $media);
    }
  }
}